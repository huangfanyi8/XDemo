#include "PluginBuilder.h"

#include "../TemplateRenderer/TemplateRenderer.h"
#include "configure_info.h"

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QUrl>

#include <filesystem>
#include <vector>

namespace
{
/**@brief  描述插件的元数据信息，根据需求自行扩展*/
struct PluginMetadata
{
    QString plugin_name{};///插件的名字
    QString plugin_version{};///插件的版本
    QString class_name{};///插件实现的类名
    QString display_name{};///显示的名字
    QString description{};///描述信息
};

bool load_metadata(const QString &metadata_file_path,PluginMetadata &metadata,QString &error_message)
{
    QFile file(metadata_file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        error_message = "Failed to open file: " + metadata_file_path;
        return false;
    }

    const QString content = QString::fromUtf8(file.readAll());
    if (content.isEmpty())
    {
        error_message = "metadata 文件为空";
        return false;
    }

    QJsonParseError parse_error{};
    const QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !doc.isObject())
    {
        error_message = "Failed to parse json file";
        return false;
    }

    const QJsonObject object = doc.object();

    metadata.plugin_name = object.value("plugin_name").toString().trimmed();
    metadata.plugin_version = object.value("plugin_version").toString().trimmed();
    metadata.class_name = object.value("class_name").toString().trimmed();
    metadata.display_name =
        object.value("display_name").toString(metadata.plugin_name).trimmed();
    metadata.description =
        object.value("description").toString("No description").trimmed();

    //根据需求来手动设置
    if (metadata.plugin_name.isEmpty()
        || metadata.plugin_version.isEmpty()
        || metadata.class_name.isEmpty())
    {
        error_message = "metadata.json is missing name, version or class_name.";
        return false;
    }

    return true;
}

} // namespace

PathSelector::PathSelector(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    edit = new QLineEdit(this);
    browse = new QPushButton("...", this);

    layout->addWidget(edit);
    layout->addWidget(browse);
}

PluginBuilderPalette::PluginBuilderPalette(QWidget *parent)
    : QWidget(parent)
{
    _setup_ui();
    setWindowTitle("PluginBuilder");
    resize(900, 650);
    _connect_signals();
}

void PluginBuilderPalette::_setup_ui()
{
    auto *main_layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    m_path_source = new PathSelector(this);
    form->addRow("Open Source Directory:", m_path_source);

    m_build = new QPushButton("Generate & Build", this);
    m_build->setStyleSheet("font-weight:bold;background:#2d5a27;color:white;");

    m_clean_build = new QPushButton("Clean & Rebuild", this);
    m_clean_build->setStyleSheet("font-weight:bold;background:#8b2727;color:white;");

    m_progress = new QProgressBar(this);
    m_log_edit = new QPlainTextEdit(this);
    m_log_edit->setReadOnly(true);
    m_log_edit->setStyleSheet(
        "background:#1e1e1e;color:#ced4da;font-family:'Consolas';");

    main_layout->addLayout(form);
    main_layout->addWidget(m_build);
    main_layout->addWidget(m_clean_build);
    main_layout->addWidget(m_progress);
    main_layout->addWidget(m_log_edit);
}

void PluginBuilderPalette::set_last_error(const QString &error)
{
    if (!error.isEmpty())
    {
        m_last_error = error;
        QMessageBox::critical(this, "Error", m_last_error);
    }
}

void PluginBuilderPalette::_connect_signals()
{
    auto trigger_build = [this](const QString &success_message)
    {
        set_last_error({});
        m_log_edit->clear();
        m_progress->setValue(0);

        if (build())
        {
            QMessageBox::information(this, "Success", success_message);
            on_open_output_dir();
        }
    };

    connect(m_build, &QPushButton::clicked, this, [trigger_build]() {
        trigger_build("Build finished!");
    });

    connect(m_clean_build, &QPushButton::clicked, this, [trigger_build]() {
        trigger_build("Clean rebuild finished!");
    });

    PathSelector::_connect_signals(std::integer_sequence<bool, true>{}, m_path_source);
}

bool PluginBuilderPalette::_execute_cmake(const QString &cmake,
                                    const QStringList &args,
                                    const QString &working_dir)
{
    QProcess process;
    process.setWorkingDirectory(working_dir);

    append_log("Working directory: " + QDir::toNativeSeparators(working_dir));

    process.start(cmake, args);
    if (!process.waitForStarted(5000))
    {
        set_last_error("Failed to start CMake: " + cmake);
        return false;
    }

    if (!process.waitForFinished(60000))
    {
        set_last_error("Timeout");
        return false;
    }



    return true;
}

bool PluginBuilderPalette::build()
{
    set_last_error({});

    const QString cmake_exe =
        QDir::fromNativeSeparators(QString::fromUtf8(env_config::cmake_path));
    const QString qt_package_dir =
        QDir::fromNativeSeparators(QString::fromUtf8(env_config::qt_path));
    const QString project_root_dir =
        QDir::fromNativeSeparators(QString::fromUtf8(env_config::project_root_path));
    const QString qt_prefix_path =
        QDir::fromNativeSeparators(QString::fromUtf8(env_config::qt_prefix_path));
    const QString template_dir = QDir(project_root_dir).filePath("templates/plugin");

    const QString source_dir = m_path_source->path();

    if (source_dir.isEmpty())
    {
        set_last_error("Source Dir cannot be empty.");
        return false;
    }

    if (!QFileInfo(source_dir).isDir())
    {
        set_last_error("Source directory not found: " + source_dir);
        return false;
    }

    const QString metadata_file_path = QDir(source_dir).filePath("metadata.json");
    PluginMetadata metadata;
    if (!load_metadata(metadata_file_path, metadata, m_last_error))
    {
        set_last_error(m_last_error);
        return false;
    }

    append_log("Source directory: " + QDir::toNativeSeparators(source_dir));
    append_log("Metadata file: " + QDir::toNativeSeparators(metadata_file_path));
    append_log(QString("Plugin: %1 (Version: %2, Class: %3)").arg(metadata.plugin_name, metadata.plugin_version, metadata.class_name));

    this->m_sandbox_dir = source_dir + "/build/" ;
    const QString build_dir = m_sandbox_dir + "/build";
    const QString built_plugin_path = m_sandbox_dir + "/plugin/" + metadata.class_name + ".dll";
    const QString publish_dir = QDir(project_root_dir).filePath("plugins/"+ metadata.plugin_name+ "/"+ metadata.plugin_version);
    const QString publish_path = QDir(publish_dir).filePath(metadata.plugin_name+ "_"+ metadata.plugin_version+ ".dll");

    this->_validate_paths(m_sandbox_dir,build_dir,publish_dir);

    append_log("Output directory: " + QDir::toNativeSeparators(m_sandbox_dir));
    append_log("Build directory: " + QDir::toNativeSeparators(build_dir));

    DongDong::TemplateRenderer renderer;
    auto context_data = kainjow::mustache::data{};

    context_data["plugin_name "] = metadata.plugin_name.toStdString();
    context_data["class_name "] = metadata.class_name.toStdString();
    context_data["plugin_name "] = metadata.plugin_name.toStdString();
    context_data["plugin_version "] = metadata.plugin_version.toStdString();
    context_data["return_type "] ="QWidget";
    DongDong::Diagnostic *diagnostic=new DongDong::Diagnostic;;
    renderer.render_to_directory(template_dir.toStdString(), m_sandbox_dir.toStdString(), context_data,diagnostic);

    m_progress->setValue(20);
    if (!_execute_cmake(cmake_exe,
            {
                "-G" , "Visual Studio 17 2022",
                "-A" , "x64",
                "-S", m_sandbox_dir,
                "-B", build_dir,
                "-DCMAKE_PREFIX_PATH=" + qt_prefix_path,
                "-DCMAKE_BUILD_TYPE=Release"
            },
            build_dir))
    {
        set_last_error("CMake configure failed: " );
        return false;
    }

    m_progress->setValue(50);
    if (!_execute_cmake(cmake_exe,{"--build", build_dir, "--config", "Release","--j 8"},build_dir))
    {
        set_last_error("CMake build failed: " );
        return false;
    }


    if (!QFile::copy(built_plugin_path, publish_path))
    {
        set_last_error("Failed to publish plugin DLL to: " + publish_path);
        return false;
    }

    m_progress->setValue(100);
    append_log("Published plugin DLL: " + QDir::toNativeSeparators(publish_path));
    return true;
}

void PluginBuilderPalette::on_open_output_dir()
{
    if (!m_sandbox_dir.isEmpty())
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_sandbox_dir));
    }
}
