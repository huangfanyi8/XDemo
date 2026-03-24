#ifndef XDEMO_TEST_PLUGINVERSIONMANAGER_H
#define XDEMO_TEST_PLUGINVERSIONMANAGER_H

#include <QPluginLoader>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <memory>
#include <vector>

class PluginInterfaceBase;


namespace xdemo::plugin_management
{

struct PluginVersionInfo
{
    QString version;
    QString description;
    QString metadata_file_path;
    QString binary_file_path;
    QStringList declared_history;

    [[nodiscard]] bool has_binary() const;
};

struct PluginInfo
{
    QString id;
    QString name;
    QString current_version;
    std::vector<PluginVersionInfo> versions;

    [[nodiscard]] QStringList switchable_versions() const;
};

class PluginVersionManager
{
public:
    PluginVersionManager() = default;
    ~PluginVersionManager();

    void load_from_directories(const QString &metadata_root, const QString &binary_root);

    [[nodiscard]] const std::vector<PluginInfo> &plugins() const;
    [[nodiscard]] const PluginInfo *plugin_at(int index) const;
    [[nodiscard]] PluginInfo *plugin_at(int index);
    [[nodiscard]] const PluginInfo *find_plugin(const QString &plugin_id) const;
    [[nodiscard]] int plugin_count() const;

    bool create_plugin(const QString &name,
                       const QString &version,
                       QString *plugin_id = nullptr,
                       QString *error_message = nullptr);

    bool rename_plugin(const QString &plugin_id,
                       const QString &name,
                       QString *error_message = nullptr);

    bool import_plugin_library(const QString &file_path,
                               QString *plugin_id = nullptr,
                               QString *error_message = nullptr);

    bool add_version(const QString &plugin_id,
                     const QString &version,
                     QString *error_message = nullptr);

    bool switch_version(const QString &plugin_id,
                        const QString &version,
                        QString *error_message = nullptr);

    bool delete_version(const QString &plugin_id,
                        const QString &version,
                        QString *error_message = nullptr);

    QWidget *create_preview_widget(const QString &plugin_id,
                                   QWidget *parent,
                                   QString *error_message = nullptr);

private:
    struct LoadedPluginLibrary
    {
        QString plugin_id;
        QString version;
        std::unique_ptr<QPluginLoader> loader;
        PluginInterfaceBase *instance = nullptr;
    };

    PluginInfo &_ensure_plugin(const QString &plugin_id, const QString &name);
    PluginInfo *_find_plugin(const QString &plugin_id);
    PluginVersionInfo *_find_version(PluginInfo &plugin, const QString &version);
    const PluginVersionInfo *_find_version(const PluginInfo &plugin, const QString &version) const;
    LoadedPluginLibrary *_find_loaded_library(const QString &plugin_id, const QString &version);

    bool _write_metadata_file(const PluginInfo &plugin,
                              PluginVersionInfo *version_info,
                              QString *error_message = nullptr);
    bool _remove_metadata_file(const PluginVersionInfo &version_info,
                               QString *error_message = nullptr);

    void _sort_plugin_versions(PluginInfo &plugin);
    void _remove_loaded_library(const QString &plugin_id, const QString &version);

    QString m_metadata_root;
    QString m_binary_root;
    std::vector<PluginInfo> m_plugins;
    std::vector<LoadedPluginLibrary> m_loaded_libraries;
};

} // namespace xdemo::plugin_management


#endif // XDEMO_TEST_PLUGINVERSIONMANAGER_H
