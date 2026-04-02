#include "PluginBuilder.h"

#include "../TemplateRenderer/TemplateRenderer.h"
#include "../TemplateRenderer/TextEncodingHelper.h"
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

struct PluginMetadata
{
    QString plugin_name;
    QString plugin_version;
    QString class_name;
    QString display_name;
    QString description;
};

bool load_metadata(const QString &metadata_file_path,
                   PluginMetadata &metadata,
                   QString &error_message)
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

    metadata.plugin_name = object.value("name").toString().trimmed();
    metadata.plugin_version = object.value("version").toString().trimmed();
    metadata.class_name = object.value("class_name").toString().trimmed();
    metadata.display_name =
        object.value("display_name").toString(metadata.plugin_name).trimmed();
    metadata.description =
        object.value("description").toString("No description").trimmed();

    if (metadata.plugin_name.isEmpty()
        || metadata.plugin_version.isEmpty()
        || metadata.class_name.isEmpty())
    {
        error_message = "metadata.json is missing name, version or class_name.";
        return false;
    }

    return true;
}

QString quote_for_log(const QString &value)
{
    QString escaped = value;
    escaped.replace('"', "\\\"");
    if (escaped.contains(' ') || escaped.contains('\t'))
    {
        return "\"" + escaped + "\"";
    }

    return escaped;
}

QString quote_for_cmd(const QString &value)
{
    QString escaped = QDir::toNativeSeparators(value);
    escaped.replace('"', "\"\"");
    return "\"" + escaped + "\"";
}

QString build_command_for_cmd(const QString &program, const QStringList &args)
{
    QStringList command_parts;
    command_parts.append(quote_for_cmd(program));

    for (const QString &arg : args)
    {
        command_parts.append(quote_for_cmd(arg));
    }

    return command_parts.join(" ");
}

QString format_command_for_log(const QString &program, const QStringList &args)
{
    QStringList formatted_args;
    for (const QString &arg : args)
    {
        formatted_args.append(quote_for_log(arg));
    }

    return QString("%1 %2").arg(quote_for_log(program), formatted_args.join(" "));
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

PluginBuilderView::PluginBuilderView(QWidget *parent)
    : QWidget(parent)
{
    _setup_ui();
    setWindowTitle("PluginBuilder");
    resize(900, 650);
    _connect_signals();
}

void PluginBuilderView::_setup_ui()
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

void PluginBuilderView::set_last_error(const QString &error)
{
    if (!error.isEmpty())
    {
        m_last_error = error;
        QMessageBox::critical(this, "Error", m_last_error);
    }
}

void PluginBuilderView::_connect_signals()
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

bool PluginBuilderView::_execute_cmake(const QString &cmd,
                                    const QStringList &args,
                                    const QString &working_dir,
                                    bool use_msvc_environment)
{
    QString program = QDir::fromNativeSeparators(cmd);
    QStringList program_args = args;

#ifdef Q_OS_WIN
    if (use_msvc_environment)
    {
        const QString vcvars_path = find_vcvars64();
        if (vcvars_path.isEmpty())
        {
            set_last_error("vcvars64.bat not found.");
            return false;
        }

        program = QDir::fromNativeSeparators(
            qEnvironmentVariable("ComSpec", "C:/Windows/System32/cmd.exe"));
        program_args = QStringList{
            "/d",
            "/c",
            "call "
                + quote_for_cmd(vcvars_path)
                + " >nul && "
                + build_command_for_cmd(cmd, args)
        };

        append_log("MSVC environment: " + QDir::toNativeSeparators(vcvars_path));
    }
#else
    Q_UNUSED(use_msvc_environment);
#endif

    QProcess process;
    process.setWorkingDirectory(working_dir);

    append_log("Working directory: " + QDir::toNativeSeparators(working_dir));
    append_log("Running: " + format_command_for_log(program, program_args));

    process.start(program, program_args);
    if (!process.waitForStarted(5000))
    {
        set_last_error("Failed to start process: " + program);
        return false;
    }

    if (!process.waitForFinished(60000))
    {
        set_last_error("Timeout");
        return false;
    }

    const QString standard_output =
        DongDong::TextEncodingHelper::decode_process_output(process.readAllStandardOutput());
    const QString standard_error =
        DongDong::TextEncodingHelper::decode_process_output(process.readAllStandardError());

    if (!standard_output.isEmpty())
    {
        append_log(standard_output);
    }

    if (!standard_error.isEmpty())
    {
        append_log(standard_error);
    }

    if (process.exitCode() != 0)
    {
        if (!standard_error.trimmed().isEmpty())
        {
            set_last_error(standard_error.trimmed());
        }
        else if (!standard_output.trimmed().isEmpty())
        {
            set_last_error(standard_output.trimmed());
        }
        else
        {
            set_last_error(QString("Process exited with code %1.")
                               .arg(process.exitCode()));
        }

        return false;
    }

    return true;
}

QString PluginBuilderView::find_vcvars64() const
{
#ifdef Q_OS_WIN
    auto try_candidate = [](const QString &path) {
        const QFileInfo file_info(path);
        if (!file_info.exists() || !file_info.isFile())
        {
            return QString{};
        }

        return QDir::fromNativeSeparators(file_info.absoluteFilePath());
    };

    const QString vs_install_dir = qEnvironmentVariable("VSINSTALLDIR");
    if (!vs_install_dir.isEmpty())
    {
        const QString vcvars_path =
            try_candidate(vs_install_dir + "/VC/Auxiliary/Build/vcvars64.bat");
        if (!vcvars_path.isEmpty())
        {
            return vcvars_path;
        }
    }

    const QString program_files_x86 = qEnvironmentVariable("ProgramFiles(x86)");
    if (!program_files_x86.isEmpty())
    {
        const QString vswhere_path =
            QDir::fromNativeSeparators(program_files_x86
                                       + "/Microsoft Visual Studio/Installer/vswhere.exe");
        if (QFileInfo(vswhere_path).exists())
        {
            QProcess process;
            process.start(vswhere_path,
                          {
                              "-latest",
                              "-products", "*",
                              "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                              "-find", "VC\\Auxiliary\\Build\\vcvars64.bat"
                          });

            if (process.waitForFinished(5000) && process.exitCode() == 0)
            {
                const QString output =
                    QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
                if (!output.isEmpty())
                {
                    const QStringList lines = output.split('\n', QString::SkipEmptyParts);
                    for (const QString &line : lines)
                    {
                        const QString vcvars_path = try_candidate(line.trimmed());
                        if (!vcvars_path.isEmpty())
                        {
                            return vcvars_path;
                        }
                    }
                }
            }
        }
    }

    const QString program_files = qEnvironmentVariable("ProgramFiles");
    const QStringList roots{
        QDir::fromNativeSeparators(program_files + "/Microsoft Visual Studio"),
        QDir::fromNativeSeparators(program_files_x86 + "/Microsoft Visual Studio")
    };
    const QStringList versions{"2022", "2019", "2017"};
    const QStringList editions{"Professional", "Community", "Enterprise", "BuildTools"};

    for (const QString &root : roots)
    {
        if (root.isEmpty())
        {
            continue;
        }

        for (const QString &version : versions)
        {
            for (const QString &edition : editions)
            {
                const QString vcvars_path =
                    try_candidate(root
                                  + "/"
                                  + version
                                  + "/"
                                  + edition
                                  + "/VC/Auxiliary/Build/vcvars64.bat");
                if (!vcvars_path.isEmpty())
                {
                    return vcvars_path;
                }
            }
        }
    }
#endif

    return {};
}

bool PluginBuilderView::build()
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
    const bool use_msvc_environment =
        qt_prefix_path.contains("msvc", Qt::CaseInsensitive);
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

    m_build_dir = source_dir + "/build/" ;
    const QString build_dir = m_build_dir + "/build";
    const QString built_plugin_path = m_build_dir + "/plugin" + metadata.class_name + ".dll";
    const QString publish_dir = QDir(project_root_dir).filePath("plugins/"+ metadata.plugin_name+ "/"+ metadata.plugin_version);
    const QString publish_path =
        QDir(publish_dir).filePath(metadata.plugin_name+ "_"+ metadata.plugin_version+ ".dll");

    this->_validate_paths(m_build_dir,build_dir,publish_dir);

    append_log("Output directory: " + QDir::toNativeSeparators(m_build_dir));
    append_log("Build directory: " + QDir::toNativeSeparators(build_dir));

    DongDong::TemplateRenderer renderer;
    DongDong::TemplateRenderer::PluginTemplateContext template_context;
    template_context.class_name = DongDong::TextEncodingHelper::to_utf8_string(metadata.class_name);
    template_context.plugin_name =
        DongDong::TextEncodingHelper::normalize_template_value(metadata.plugin_name);
    template_context.plugin_version =
        DongDong::TextEncodingHelper::normalize_template_value(metadata.plugin_version);
    template_context.label_text = DongDong::TextEncodingHelper::normalize_template_value(
        DongDong::TextEncodingHelper::escape_cpp_u8_string_literal(
            metadata.display_name + " - " + metadata.description));

    std::vector<std::filesystem::path> created_files;
    m_progress->setValue(10);
    if (!renderer.generate_plugin_files(
            DongDong::TextEncodingHelper::to_filesystem_path(m_build_dir),
            DongDong::TextEncodingHelper::to_filesystem_path(template_dir),
            DongDong::TextEncodingHelper::to_filesystem_path(metadata_file_path),
            template_context,
            &created_files))
    {
        set_last_error(QString::fromUtf8(renderer.get_error().c_str()));
        return false;
    }

    for (const auto &created_file : created_files)
    {
        append_log("Created: "
                   + DongDong::TextEncodingHelper::from_filesystem_path(created_file));
    }

    m_progress->setValue(20);
    if (!_execute_cmake(cmake_exe,
                     {
                         "-S", m_build_dir,
                         "-B", build_dir,
                         "-DCMAKE_PREFIX_PATH=" + qt_prefix_path,
                         "-DCMAKE_BUILD_TYPE=Release"
                     },
                     build_dir,
                     use_msvc_environment))
    {
        set_last_error("CMake configure failed: " );
        return false;
    }

    m_progress->setValue(50);
    if (!_execute_cmake(cmake_exe,
                     {"--build", build_dir, "--config", "Release"},
                     build_dir,
                     use_msvc_environment))
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

void PluginBuilderView::on_open_output_dir()
{
    if (!m_build_dir.isEmpty())
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_build_dir));
    }
}
