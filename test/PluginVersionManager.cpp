#include "PluginVersionManager.h"

#include "../plugins/include/PluginInterfaceBase.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <algorithm>


namespace xdemo::plugin_management
{
namespace _details
{

struct ParsedPluginMetadata
{
    QString plugin_id;
    QString name;
    PluginVersionInfo version_info;
};

QString normalize_plugin_id(const QString &value)
{
    QString normalized = value.trimmed().toLower();
    normalized.replace(QRegularExpression("[^\\p{L}\\p{N}]+"), QStringLiteral("_"));
    normalized.remove(QRegularExpression("^_+"));
    normalized.remove(QRegularExpression("_+$"));

    if (normalized.isEmpty())
    {
        return QStringLiteral("plugin");
    }

    return normalized;
}

QString sanitize_file_component(const QString &value)
{
    QString sanitized = value.trimmed();
    sanitized.replace(QRegularExpression("[\\\\/:*?\"<>|\\s]+"), QStringLiteral("_"));
    sanitized.remove(QRegularExpression("^_+"));
    sanitized.remove(QRegularExpression("_+$"));

    if (sanitized.isEmpty())
    {
        return QStringLiteral("plugin");
    }

    return sanitized;
}

std::vector<QString> split_version(const QString &version)
{
    const QStringList parts = version.split(QRegularExpression("[\\.\\-_]"), QString::SkipEmptyParts);
    return std::vector<QString>(parts.begin(), parts.end());
}

bool is_number(const QString &value)
{
    bool ok = false;
    value.toInt(&ok);
    return ok;
}

int compare_versions(const QString &left, const QString &right)
{
    const std::vector<QString> left_parts = split_version(left);
    const std::vector<QString> right_parts = split_version(right);
    const std::size_t max_size = std::max(left_parts.size(), right_parts.size());

    for (std::size_t index = 0; index < max_size; ++index)
    {
        const QString left_part = index < left_parts.size() ? left_parts[index] : QStringLiteral("0");
        const QString right_part = index < right_parts.size() ? right_parts[index] : QStringLiteral("0");

        const bool left_is_number = is_number(left_part);
        const bool right_is_number = is_number(right_part);

        if (left_is_number && right_is_number)
        {
            const int left_value = left_part.toInt();
            const int right_value = right_part.toInt();

            if (left_value != right_value)
            {
                return left_value < right_value ? -1 : 1;
            }

            continue;
        }

        const int compare_result = QString::compare(left_part, right_part, Qt::CaseInsensitive);
        if (compare_result != 0)
        {
            return compare_result < 0 ? -1 : 1;
        }
    }

    return 0;
}

QStringList json_array_to_string_list(const QJsonArray &array)
{
    QStringList values;

    for (const QJsonValue &value : array)
    {
        const QString item = value.toString().trimmed();
        if (!item.isEmpty())
        {
            values.append(item);
        }
    }

    values.removeDuplicates();
    return values;
}

QString find_binary_file(const QString &binary_root, const QString &base_name)
{
    if (binary_root.trimmed().isEmpty())
    {
        return QString();
    }

    const QStringList suffixes =
    {
        QStringLiteral(".dll"),
        QStringLiteral(".so"),
        QStringLiteral(".dylib")
    };

    QDir binary_dir(binary_root);
    for (const QString &suffix : suffixes)
    {
        const QString candidate = binary_dir.absoluteFilePath(base_name + suffix);
        if (QFileInfo::exists(candidate))
        {
            return QDir::cleanPath(candidate);
        }
    }

    return QString();
}

QJsonObject build_metadata_object(const PluginInfo &plugin, const PluginVersionInfo &version_info)
{
    QJsonObject object;
    object.insert(QStringLiteral("name"), plugin.name);
    object.insert(QStringLiteral("version"), version_info.version);

    if (!version_info.description.trimmed().isEmpty())
    {
        object.insert(QStringLiteral("description"), version_info.description);
    }

    if (!version_info.declared_history.isEmpty())
    {
        object.insert(QStringLiteral("history"), QJsonArray::fromStringList(version_info.declared_history));
    }

    return object;
}

bool read_metadata_file(const QString &file_path,
                        const QString &metadata_root,
                        const QString &binary_root,
                        ParsedPluginMetadata *metadata)
{
    if (metadata == nullptr)
    {
        return false;
    }

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
    {
        return false;
    }

    const QJsonObject object = document.object();
    const QString name = object.value(QStringLiteral("name")).toString().trimmed();
    const QString version = object.value(QStringLiteral("version")).toString().trimmed();
    if (version.isEmpty())
    {
        return false;
    }

    const QFileInfo file_info(file_path);
    const QDir metadata_dir(metadata_root);
    const QString relative_parent = metadata_dir.relativeFilePath(file_info.dir().absolutePath());
    const QString plugin_key = relative_parent == QStringLiteral(".")
                               ? (name.isEmpty() ? file_info.completeBaseName() : name)
                               : relative_parent;

    metadata->plugin_id = normalize_plugin_id(plugin_key);
    metadata->name = name.isEmpty() ? file_info.completeBaseName() : name;
    metadata->version_info.version = version;
    metadata->version_info.description = object.value(QStringLiteral("description")).toString().trimmed();
    metadata->version_info.metadata_file_path = QDir::cleanPath(file_info.absoluteFilePath());
    metadata->version_info.binary_file_path = find_binary_file(binary_root, file_info.completeBaseName());
    metadata->version_info.declared_history = json_array_to_string_list(object.value(QStringLiteral("history")).toArray());
    return true;
}

void sort_plugins(std::vector<PluginInfo> *plugins)
{
    if (plugins == nullptr)
    {
        return;
    }

    std::sort(plugins->begin(), plugins->end(),
              [](const PluginInfo &left, const PluginInfo &right)
              {
                  return QString::compare(left.name, right.name, Qt::CaseInsensitive) < 0;
              });
}

} // namespace _details

bool PluginVersionInfo::has_binary() const
{
    return !binary_file_path.trimmed().isEmpty();
}

QStringList PluginInfo::switchable_versions() const
{
    QStringList versions_list;

    for (const PluginVersionInfo &version_info : versions)
    {
        if (version_info.version != current_version)
        {
            versions_list.append(version_info.version);
        }
    }

    return versions_list;
}

PluginVersionManager::~PluginVersionManager() = default;

void PluginVersionManager::load_from_directories(const QString &metadata_root, const QString &binary_root)
{
    m_metadata_root = QDir::cleanPath(metadata_root);
    m_binary_root = QDir::cleanPath(binary_root);
    m_plugins.clear();
    m_loaded_libraries.clear();

    QDir metadata_dir(m_metadata_root);
    if (!metadata_dir.exists())
    {
        return;
    }

    QDirIterator iterator(m_metadata_root,
                          QStringList() << QStringLiteral("*.json"),
                          QDir::Files,
                          QDirIterator::Subdirectories);

    while (iterator.hasNext())
    {
        const QString file_path = iterator.next();
        _details::ParsedPluginMetadata parsed_metadata;

        if (!_details::read_metadata_file(file_path, m_metadata_root, m_binary_root, &parsed_metadata))
        {
            continue;
        }

        PluginInfo &plugin = _ensure_plugin(parsed_metadata.plugin_id, parsed_metadata.name);
        if (plugin.name.trimmed().isEmpty())
        {
            plugin.name = parsed_metadata.name;
        }

        PluginVersionInfo *existing_version = _find_version(plugin, parsed_metadata.version_info.version);
        if (existing_version == nullptr)
        {
            plugin.versions.push_back(parsed_metadata.version_info);
        }
        else
        {
            *existing_version = parsed_metadata.version_info;
        }
    }

    for (PluginInfo &plugin : m_plugins)
    {
        _sort_plugin_versions(plugin);

        if (plugin.current_version.isEmpty() && !plugin.versions.empty())
        {
            plugin.current_version = plugin.versions.front().version;
        }
    }

    _details::sort_plugins(&m_plugins);
}

const std::vector<PluginInfo> &PluginVersionManager::plugins() const
{
    return m_plugins;
}

const PluginInfo *PluginVersionManager::plugin_at(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_plugins.size()))
    {
        return nullptr;
    }

    return &m_plugins[static_cast<std::size_t>(index)];
}

PluginInfo *PluginVersionManager::plugin_at(int index)
{
    if (index < 0 || index >= static_cast<int>(m_plugins.size()))
    {
        return nullptr;
    }

    return &m_plugins[static_cast<std::size_t>(index)];
}

const PluginInfo *PluginVersionManager::find_plugin(const QString &plugin_id) const
{
    for (const PluginInfo &plugin : m_plugins)
    {
        if (plugin.id == plugin_id)
        {
            return &plugin;
        }
    }

    return nullptr;
}

int PluginVersionManager::plugin_count() const
{
    return static_cast<int>(m_plugins.size());
}

bool PluginVersionManager::create_plugin(const QString &name,
                                         const QString &version,
                                         QString *plugin_id,
                                         QString *error_message)
{
    const QString trimmed_name = name.trimmed();
    const QString trimmed_version = version.trimmed();

    if (trimmed_name.isEmpty())
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("插件名称不能为空。");
        }

        return false;
    }

    if (trimmed_version.isEmpty())
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("默认版本不能为空。");
        }

        return false;
    }

    const QString normalized_plugin_id = _details::normalize_plugin_id(trimmed_name);
    if (_find_plugin(normalized_plugin_id) != nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("同名插件已存在。");
        }

        return false;
    }

    PluginInfo plugin;
    plugin.id = normalized_plugin_id;
    plugin.name = trimmed_name;
    plugin.current_version = trimmed_version;

    PluginVersionInfo version_info;
    version_info.version = trimmed_version;
    plugin.versions.push_back(version_info);

    m_plugins.push_back(plugin);
    PluginInfo *created_plugin = &m_plugins.back();

    if (!_write_metadata_file(*created_plugin, &created_plugin->versions.front(), error_message))
    {
        m_plugins.pop_back();
        return false;
    }

    _sort_plugin_versions(*created_plugin);
    _details::sort_plugins(&m_plugins);

    if (plugin_id != nullptr)
    {
        *plugin_id = normalized_plugin_id;
    }

    return true;
}

bool PluginVersionManager::rename_plugin(const QString &plugin_id,
                                         const QString &name,
                                         QString *error_message)
{
    PluginInfo *plugin = _find_plugin(plugin_id);
    if (plugin == nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("未找到插件。");
        }

        return false;
    }

    const QString trimmed_name = name.trimmed();
    if (trimmed_name.isEmpty())
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("插件名称不能为空。");
        }

        return false;
    }

    plugin->name = trimmed_name;
    for (PluginVersionInfo &version_info : plugin->versions)
    {
        if (!_write_metadata_file(*plugin, &version_info, error_message))
        {
            return false;
        }
    }

    _details::sort_plugins(&m_plugins);
    return true;
}

bool PluginVersionManager::import_plugin_library(const QString &file_path,
                                                 QString *plugin_id,
                                                 QString *error_message)
{
    auto loader = std::make_unique<QPluginLoader>(file_path);
    QObject *instance_object = loader->instance();
    if (instance_object == nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = loader->errorString();
        }

        return false;
    }

    PluginInterfaceBase *plugin_interface = qobject_cast<PluginInterfaceBase *>(instance_object);
    if (plugin_interface == nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("插件未实现 PluginInterfaceBase 接口。");
        }

        return false;
    }

    const QJsonObject metadata_root = loader->metaData();
    const QJsonObject metadata_object = metadata_root.value(QStringLiteral("MetaData")).toObject();

    const QString name = metadata_object.value(QStringLiteral("name")).toString().trimmed().isEmpty()
                         ? plugin_interface->name().trimmed()
                         : metadata_object.value(QStringLiteral("name")).toString().trimmed();

    const QString version = metadata_object.value(QStringLiteral("version")).toString().trimmed().isEmpty()
                            ? plugin_interface->version().trimmed()
                            : metadata_object.value(QStringLiteral("version")).toString().trimmed();

    QStringList history_versions = _details::json_array_to_string_list(metadata_object.value(QStringLiteral("history")).toArray());
    if (history_versions.isEmpty())
    {
        history_versions = plugin_interface->history();
        history_versions.removeDuplicates();
    }

    if (name.isEmpty() || version.isEmpty())
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("插件元数据缺少 name 或 version。");
        }

        return false;
    }

    const QString normalized_plugin_id = _details::normalize_plugin_id(name);
    PluginInfo &plugin = _ensure_plugin(normalized_plugin_id, name);
    plugin.name = name;

    PluginVersionInfo *version_info = _find_version(plugin, version);
    if (version_info == nullptr)
    {
        PluginVersionInfo new_version_info;
        new_version_info.version = version;
        new_version_info.binary_file_path = QDir::cleanPath(QFileInfo(file_path).absoluteFilePath());
        new_version_info.declared_history = history_versions;
        plugin.versions.push_back(new_version_info);
    }
    else
    {
        version_info->binary_file_path = QDir::cleanPath(QFileInfo(file_path).absoluteFilePath());
        version_info->declared_history = history_versions;
    }

    plugin.current_version = version;
    _sort_plugin_versions(plugin);
    _details::sort_plugins(&m_plugins);

    LoadedPluginLibrary *loaded_library = _find_loaded_library(normalized_plugin_id, version);
    if (loaded_library == nullptr)
    {
        LoadedPluginLibrary new_library;
        new_library.plugin_id = normalized_plugin_id;
        new_library.version = version;
        new_library.instance = plugin_interface;
        new_library.loader = std::move(loader);
        m_loaded_libraries.push_back(std::move(new_library));
    }
    else
    {
        loaded_library->instance = plugin_interface;
        loaded_library->loader = std::move(loader);
    }

    if (plugin_id != nullptr)
    {
        *plugin_id = normalized_plugin_id;
    }

    return true;
}

bool PluginVersionManager::add_version(const QString &plugin_id,
                                       const QString &version,
                                       QString *error_message)
{
    PluginInfo *plugin = _find_plugin(plugin_id);
    if (plugin == nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("未找到插件。");
        }

        return false;
    }

    const QString trimmed_version = version.trimmed();
    if (trimmed_version.isEmpty())
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("版本号不能为空。");
        }

        return false;
    }

    if (_find_version(*plugin, trimmed_version) != nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("该版本已存在。");
        }

        return false;
    }

    PluginVersionInfo version_info;
    version_info.version = trimmed_version;
    plugin->versions.push_back(version_info);

    if (!_write_metadata_file(*plugin, &plugin->versions.back(), error_message))
    {
        plugin->versions.pop_back();
        return false;
    }

    _sort_plugin_versions(*plugin);
    return true;
}

bool PluginVersionManager::switch_version(const QString &plugin_id,
                                          const QString &version,
                                          QString *error_message)
{
    PluginInfo *plugin = _find_plugin(plugin_id);
    if (plugin == nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("未找到插件。");
        }

        return false;
    }

    if (_find_version(*plugin, version) == nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("目标版本不存在。");
        }

        return false;
    }

    plugin->current_version = version;
    return true;
}

bool PluginVersionManager::delete_version(const QString &plugin_id,
                                          const QString &version,
                                          QString *error_message)
{
    PluginInfo *plugin = _find_plugin(plugin_id);
    if (plugin == nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("未找到插件。");
        }

        return false;
    }

    if (plugin->current_version == version)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("不能删除当前版本。");
        }

        return false;
    }

    auto iterator = plugin->versions.begin();
    while (iterator != plugin->versions.end())
    {
        if (iterator->version == version)
        {
            break;
        }

        ++iterator;
    }

    if (iterator == plugin->versions.end())
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("目标版本不存在。");
        }

        return false;
    }

    const PluginVersionInfo removed_version = *iterator;
    plugin->versions.erase(iterator);

    if (!_remove_metadata_file(removed_version, error_message))
    {
        plugin->versions.push_back(removed_version);
        _sort_plugin_versions(*plugin);
        return false;
    }

    _remove_loaded_library(plugin_id, version);
    return true;
}

QWidget *PluginVersionManager::create_preview_widget(const QString &plugin_id,
                                                     QWidget *parent,
                                                     QString *error_message)
{
    PluginInfo *plugin = _find_plugin(plugin_id);
    if (plugin == nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("未找到插件。");
        }

        return nullptr;
    }

    PluginVersionInfo *version_info = _find_version(*plugin, plugin->current_version);
    if (version_info == nullptr)
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("当前版本不存在。");
        }

        return nullptr;
    }

    if (!version_info->has_binary())
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("当前版本没有对应的插件库文件。");
        }

        return nullptr;
    }

    LoadedPluginLibrary *loaded_library = _find_loaded_library(plugin_id, version_info->version);
    if (loaded_library == nullptr)
    {
        auto loader = std::make_unique<QPluginLoader>(version_info->binary_file_path);
        QObject *instance_object = loader->instance();
        if (instance_object == nullptr)
        {
            if (error_message != nullptr)
            {
                *error_message = loader->errorString();
            }

            return nullptr;
        }

        PluginInterfaceBase *plugin_interface = qobject_cast<PluginInterfaceBase *>(instance_object);
        if (plugin_interface == nullptr)
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("插件未实现 PluginInterfaceBase 接口。");
            }

            return nullptr;
        }

        LoadedPluginLibrary new_library;
        new_library.plugin_id = plugin_id;
        new_library.version = version_info->version;
        new_library.instance = plugin_interface;
        new_library.loader = std::move(loader);
        m_loaded_libraries.push_back(std::move(new_library));
        loaded_library = &m_loaded_libraries.back();
    }

    QWidget *widget = loaded_library->instance->create_widget(parent);
    if (widget == nullptr && error_message != nullptr)
    {
        *error_message = QStringLiteral("插件没有返回可预览控件。");
    }

    return widget;
}

PluginInfo &PluginVersionManager::_ensure_plugin(const QString &plugin_id, const QString &name)
{
    if (PluginInfo *plugin = _find_plugin(plugin_id))
    {
        if (plugin->name.trimmed().isEmpty() && !name.trimmed().isEmpty())
        {
            plugin->name = name.trimmed();
        }

        return *plugin;
    }

    PluginInfo plugin;
    plugin.id = plugin_id;
    plugin.name = name.trimmed();
    m_plugins.push_back(plugin);
    return m_plugins.back();
}

PluginInfo *PluginVersionManager::_find_plugin(const QString &plugin_id)
{
    for (PluginInfo &plugin : m_plugins)
    {
        if (plugin.id == plugin_id)
        {
            return &plugin;
        }
    }

    return nullptr;
}

PluginVersionInfo *PluginVersionManager::_find_version(PluginInfo &plugin, const QString &version)
{
    for (PluginVersionInfo &version_info : plugin.versions)
    {
        if (version_info.version == version)
        {
            return &version_info;
        }
    }

    return nullptr;
}

const PluginVersionInfo *PluginVersionManager::_find_version(const PluginInfo &plugin, const QString &version) const
{
    for (const PluginVersionInfo &version_info : plugin.versions)
    {
        if (version_info.version == version)
        {
            return &version_info;
        }
    }

    return nullptr;
}

PluginVersionManager::LoadedPluginLibrary *PluginVersionManager::_find_loaded_library(const QString &plugin_id,
                                                                                       const QString &version)
{
    for (LoadedPluginLibrary &loaded_library : m_loaded_libraries)
    {
        if (loaded_library.plugin_id == plugin_id && loaded_library.version == version)
        {
            return &loaded_library;
        }
    }

    return nullptr;
}

bool PluginVersionManager::_write_metadata_file(const PluginInfo &plugin,
                                                PluginVersionInfo *version_info,
                                                QString *error_message)
{
    if (version_info == nullptr)
    {
        return false;
    }

    if (m_metadata_root.trimmed().isEmpty())
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("插件元数据目录未配置。");
        }

        return false;
    }

    QDir metadata_root_dir(m_metadata_root);
    if (!metadata_root_dir.exists() && !metadata_root_dir.mkpath(QStringLiteral(".")))
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("无法创建插件元数据目录。");
        }

        return false;
    }

    const QString plugin_directory_name = plugin.id.trimmed().isEmpty()
                                          ? _details::normalize_plugin_id(plugin.name)
                                          : plugin.id;
    if (!metadata_root_dir.mkpath(plugin_directory_name))
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("无法创建插件目录。");
        }

        return false;
    }

    QDir plugin_directory(metadata_root_dir.absoluteFilePath(plugin_directory_name));
    QString metadata_file_path = version_info->metadata_file_path.trimmed();
    if (metadata_file_path.isEmpty())
    {
        const QString file_name = _details::sanitize_file_component(plugin_directory_name)
                                  + QStringLiteral("_")
                                  + _details::sanitize_file_component(version_info->version)
                                  + QStringLiteral(".json");
        metadata_file_path = plugin_directory.absoluteFilePath(file_name);
    }

    QFile metadata_file(metadata_file_path);
    if (!metadata_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (error_message != nullptr)
        {
            *error_message = QStringLiteral("无法写入插件元数据文件。");
        }

        return false;
    }

    const QJsonDocument document(_details::build_metadata_object(plugin, *version_info));
    metadata_file.write(document.toJson(QJsonDocument::Indented));
    metadata_file.close();

    version_info->metadata_file_path = QDir::cleanPath(metadata_file_path);
    return true;
}

bool PluginVersionManager::_remove_metadata_file(const PluginVersionInfo &version_info,
                                                 QString *error_message)
{
    const QString metadata_file_path = version_info.metadata_file_path.trimmed();
    if (metadata_file_path.isEmpty())
    {
        return true;
    }

    QFile metadata_file(metadata_file_path);
    if (!metadata_file.exists())
    {
        return true;
    }

    if (metadata_file.remove())
    {
        return true;
    }

    if (error_message != nullptr)
    {
        *error_message = QStringLiteral("无法删除插件元数据文件。");
    }

    return false;
}

void PluginVersionManager::_sort_plugin_versions(PluginInfo &plugin)
{
    std::sort(plugin.versions.begin(),
              plugin.versions.end(),
              [](const PluginVersionInfo &left, const PluginVersionInfo &right)
              {
                  return _details::compare_versions(left.version, right.version) > 0;
              });
}

void PluginVersionManager::_remove_loaded_library(const QString &plugin_id, const QString &version)
{
    const auto iterator = std::remove_if(m_loaded_libraries.begin(),
                                         m_loaded_libraries.end(),
                                         [&plugin_id, &version](const LoadedPluginLibrary &loaded_library)
                                         {
                                             return loaded_library.plugin_id == plugin_id
                                                    && loaded_library.version == version;
                                         });

    m_loaded_libraries.erase(iterator, m_loaded_libraries.end());
}

} // namespace xdemo::plugin_management
