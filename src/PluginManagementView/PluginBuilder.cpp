#include "PluginBuilder.h"
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>
#include <QTemporaryFile>
#include <QTextCodec>
#include "../../mustache/mustache.hpp"
#include "configure_info.h"

namespace _details
{
/**
 * @brief 解码外部进程输出文本。
 * @param output 原始字节流。
 * @return 解码后的字符串。
 */
QString decode_process_output(const QByteArray &output)
{
#ifdef Q_OS_WIN
    if (output.isEmpty())
    {
        return {};
    }

    QTextCodec::ConverterState state;
    QTextCodec *utf8_codec = QTextCodec::codecForName("UTF-8");
    if (utf8_codec != nullptr)
    {
        const QString utf8_text =
            utf8_codec->toUnicode(output.constData(), output.size(), &state);
        if (state.invalidChars == 0 && utf8_text.toUtf8() == output)
        {
            return utf8_text;
        }
    }

    return QString::fromLocal8Bit(output);
#else
    return QString::fromUtf8(output);
#endif
}

/**
 * @brief 转义将写入 C++ `u8` 字符串字面量的文本。
 * @param value 原始文本。
 * @return 转义后的字符串。
 */
QString escape_cpp_u8_string_literal(const QString &value)
{
    QString escaped;
    const QVector<uint> code_points = value.toUcs4();
    escaped.reserve(static_cast<int>(code_points.size()));

    for (const uint code_point : code_points)
    {
        switch (code_point)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        default:
            if (code_point >= 0x20 && code_point <= 0x7E)
            {
                escaped += QChar(static_cast<ushort>(code_point));
            }
            else if (code_point <= 0xFFFF)
            {
                escaped += QString("\\u%1").arg(code_point, 4, 16, QChar('0')).toUpper();
            }
            else
            {
                escaped += QString("\\U%1").arg(code_point, 8, 16, QChar('0')).toUpper();
            }
            break;
        }
    }

    return escaped;
}

/**
 * @brief 读取 UTF-8 文本文件。
 * @param path 文件路径。
 * @param error_message 读取失败时输出错误信息。
 * @return 文件内容；失败时返回空字符串。
 */
QString read_text_file(const QString &path, QString &error_message)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        error_message = "Failed to open file: " + path;
        return {};
    }

    return QString::fromUtf8(file.readAll());
}

/**
 * @brief 读取 JSON 文件对象。
 * @param path 文件路径。
 * @param object 输出的 JSON 对象。
 * @param error_message 失败时输出错误信息。
 * @return 读取成功时返回 true，否则返回 false。
 */
bool read_json_object(const QString &path, QJsonObject &object, QString &error_message)
{
    const QString content = read_text_file(path, error_message);
    if (content.isEmpty() && !error_message.isEmpty())
    {
        return false;
    }

    QJsonParseError parse_error{};
    const QJsonDocument document =
        QJsonDocument::fromJson(content.toUtf8(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError)
    {
        error_message = "Failed to parse json file: " + path + " - " + parse_error.errorString();
        return false;
    }

    if (!document.isObject())
    {
        error_message = "Json root must be an object: " + path;
        return false;
    }

    object = document.object();
    return true;
}

/**
 * @brief 使用 mustache 模板渲染文本。
 * @param tpl 模板文本。
 * @param vars 模板变量。
 * @return 渲染后的结果。
 */
QString render_template(const QString &tpl, const QVariantMap &vars)
{
    using namespace kainjow::mustache;
    data d;
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it)
    {
        QString value = it.value().toString();
        if (value.contains('\\'))
        {
            value = QDir::fromNativeSeparators(value);
        }

        d[it.key().toStdString()] = value.toStdString();
    }

    mustache tmpl{tpl.toStdString()};
    return QString::fromStdString(tmpl.render(d));
}
}

PathSelector::PathSelector(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);

    edit = new QLineEdit(this);
    browse = new QPushButton("...", this);

    layout->addWidget(edit);
    layout->addWidget(browse);
}


PluginBuilderView::PluginBuilderView(QWidget *parent)
    : QWidget(parent)
{
    setup_ui();
    setWindowTitle("PluginBuilder");
    resize(900, 650);
    _connect_signals();
}

void PluginBuilderView::setup_ui()
{
    auto *main_layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    m_path_source = new PathSelector;
    m_path_cmake  = new PathSelector;
    m_path_qt     = new PathSelector;

    m_path_cmake->edit->setReadOnly(true);
    m_path_qt->edit->setReadOnly(true);
    m_path_cmake->browse->setEnabled(false);
    m_path_qt->browse->setEnabled(false);

    form->addRow("Source Directory:",   m_path_source);
    form->addRow("CMake Executable (configure_info.h):", m_path_cmake);
    form->addRow("Qt Path (configure_info.h):", m_path_qt);

    m_build = new QPushButton("Generate & Build", this);
    m_build->setStyleSheet("font-weight:bold;background:#2d5a27;color:white;");

    m_open_dir = new QPushButton("Open Output Folder", this);
    m_open_dir->setEnabled(false);

    m_progress = new QProgressBar(this);
    m_log_edit = new QPlainTextEdit(this);
    m_log_edit->setReadOnly(true);
    m_log_edit->setStyleSheet(
        "background:#1e1e1e;color:#ced4da;font-family:'Consolas';");

    main_layout->addLayout(form);
    main_layout->addWidget(m_build);
    main_layout->addWidget(m_open_dir);
    main_layout->addWidget(m_progress);
    main_layout->addWidget(m_log_edit);

    m_clean_build = new QPushButton("Clean & Rebuild", this);
    m_clean_build->setStyleSheet("font-weight:bold;background:#8b2727;color:white;");

    main_layout->addWidget(m_clean_build);


}


void PluginBuilderView::append_log(const QString &log)
{
    m_log_edit->appendPlainText(log);
}

bool PluginBuilderView::write_file(const QString &path, const QString &content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_last_error = "Failed to write file: " + path;
        return false;
    }

    const QString suffix = QFileInfo(path).suffix().toLower();
    const bool is_cpp_source = suffix == "h"
                               || suffix == "hpp"
                               || suffix == "c"
                               || suffix == "cc"
                               || suffix == "cpp"
                               || suffix == "cxx";

    if (is_cpp_source)
    {
        static const QByteArray utf8_bom("\xEF\xBB\xBF", 3);
        f.write(utf8_bom);
    }

    f.write(content.toUtf8());
    return true;
}

bool PluginBuilderView::run_process(const QString &cmd,
                                    const QStringList &args,
                                    const QString &working_dir,
                                    bool use_msvc_environment)
{
    auto quote_argument = [](const QString &value) {
        QString escaped = value;
        escaped.replace('"', "\\\"");
        if (escaped.contains(' ') || escaped.contains('\t'))
        {
            return "\"" + escaped + "\"";
        }
        return escaped;
    };

    QString program = cmd;
    QStringList program_args = args;

#ifdef Q_OS_WIN
    if (use_msvc_environment)
    {
        auto quote_for_batch = [](const QString &value) {
            QString escaped = QDir::toNativeSeparators(value);
            escaped.replace('"', "\"\"");
            if (escaped.contains(' ') || escaped.contains('\t'))
            {
                return "\"" + escaped + "\"";
            }
            return escaped;
        };

        const QString vcvars_path = find_vcvars64();
        if (vcvars_path.isEmpty())
        {
            m_last_error = "vcvars64.bat not found.";
            return false;
        }

        QStringList formatted_batch_args;
        for (const QString &arg : args)
        {
            formatted_batch_args.append(quote_for_batch(arg));
        }

        const QString command_line = formatted_batch_args.isEmpty()
            ? quote_for_batch(cmd)
            : quote_for_batch(cmd) + " " + formatted_batch_args.join(" ");

        QTemporaryFile script_file(QDir::tempPath() + "/PluginBuilder-XXXXXX.cmd");
        if (!script_file.open())
        {
            m_last_error = "Failed to create temporary MSVC script.";
            return false;
        }

        const QString script_content =
            "@echo off\r\n"
            "call " + quote_for_batch(vcvars_path) + " >nul\r\n"
            "if errorlevel 1 exit /b %errorlevel%\r\n"
            + command_line + "\r\n";

        script_file.write(script_content.toLocal8Bit());
        script_file.flush();

        program = QDir::fromNativeSeparators(
            qEnvironmentVariable("ComSpec", "C:/Windows/System32/cmd.exe"));
        program_args = QStringList{
            "/d",
            "/c",
            QDir::fromNativeSeparators(script_file.fileName())
        };

        append_log("MSVC environment script: " + QDir::toNativeSeparators(vcvars_path));
    }
#else
    Q_UNUSED(use_msvc_environment);
#endif

    QStringList formatted_args;
    for (const QString &arg : program_args)
    {
        formatted_args.append(quote_argument(arg));
    }

    QProcess p;
    p.setWorkingDirectory(working_dir);
    append_log("Working directory: " + QDir::toNativeSeparators(working_dir));
    append_log(QString("Running: %1 %2")
               .arg(quote_argument(program), formatted_args.join(" ")));
    p.start(QDir::fromNativeSeparators(program), program_args);
    if (!p.waitForFinished(60000))
    {
        m_last_error = "Timeout";
        return false;
    }
    const QString standard_output =
        _details::decode_process_output(p.readAllStandardOutput());
    const QString standard_error =
        _details::decode_process_output(p.readAllStandardError());

    if (!standard_output.isEmpty())
    {
        append_log(standard_output);
    }

    if (!standard_error.isEmpty())
    {
        append_log(standard_error);
    }
    if (p.exitCode() != 0)
    {
        if (!standard_error.trimmed().isEmpty())
        {
            m_last_error = standard_error.trimmed();
        }
        else if (!standard_output.trimmed().isEmpty())
        {
            m_last_error = standard_output.trimmed();
        }
        else
        {
            m_last_error = QString("Process exited with code %1.").arg(p.exitCode());
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
        if (!file_info.exists() || !file_info.isFile()) {
            return QString{};
        }

        return QDir::fromNativeSeparators(file_info.absoluteFilePath());
    };

    const QString vs_install_dir = qEnvironmentVariable("VSINSTALLDIR");
    if (!vs_install_dir.isEmpty()) {
        const QString vcvars_path =
            try_candidate(vs_install_dir + "/VC/Auxiliary/Build/vcvars64.bat");
        if (!vcvars_path.isEmpty()) {
            return vcvars_path;
        }
    }

    const QString program_files_x86 = qEnvironmentVariable("ProgramFiles(x86)");
    if (!program_files_x86.isEmpty()) {
        const QString vswhere_path =
            QDir::fromNativeSeparators(program_files_x86
                                       + "/Microsoft Visual Studio/Installer/vswhere.exe");
        if (QFileInfo(vswhere_path).exists()) {
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
                const QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
                if (!output.isEmpty())
                {
                    const QStringList lines = output.split('\n', QString::SkipEmptyParts);
                    for (const QString &line : lines)
                    {
                        const QString vcvars_path = try_candidate(line.trimmed());
                        if (!vcvars_path.isEmpty())
                            return vcvars_path;
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

    for (const QString &root : roots) {
        if (root.isEmpty()) {
            continue;
        }

        for (const QString &version : versions) {
            for (const QString &edition : editions) {
                const QString vcvars_path =
                    try_candidate(root + "/" + version + "/" + edition + "/VC/Auxiliary/Build/vcvars64.bat");
                if (!vcvars_path.isEmpty()) {
                    return vcvars_path;
                }
            }
        }
    }
#endif

    return {};
}

bool PluginBuilderView::generate_plugin_files(const QString &out_dir,
                                              const QString &template_dir,
                                              const QString &metadata_file_path,
                                              const QJsonObject &metadata)
{
    const QString class_name = metadata.value("className").toString().trimmed();
    const QString plugin_name = metadata.value("name").toString().trimmed();
    const QString plugin_version = metadata.value("version").toString().trimmed();
    const QString display_name =
        metadata.value("displayName").toString(plugin_name).trimmed();
    const QString description =
        metadata.value("description").toString("No description").trimmed();

    auto load_template = [this](const QString &path, QString &content) {
        QString error_message;
        content = _details::read_text_file(path, error_message);
        if (content.isEmpty() && !error_message.isEmpty())
        {
            m_last_error = error_message;
            return false;
        }
        return true;
    };

    QString interface_template;
    QString header_template;
    QString source_template;
    QString cmake_template;

    if (!load_template(QDir(template_dir).filePath("PluginInterfaceBase.h.in"), interface_template)
        || !load_template(QDir(template_dir).filePath("PluginClass.h.in"), header_template)
        || !load_template(QDir(template_dir).filePath("PluginClass.cpp.in"), source_template)
        || !load_template(QDir(template_dir).filePath("CMakeLists.txt.in"), cmake_template))
    {
        return false;
    }

    QVariantMap template_vars;
    template_vars["CLASS_NAME"] = class_name;
    template_vars["PLUGIN_NAME"] = plugin_name;
    template_vars["VERSION"] = plugin_version;
    template_vars["LABEL_TEXT"] =
        _details::escape_cpp_u8_string_literal(display_name + " - " + description);

    if (!write_file(QDir(out_dir).filePath("PluginInterfaceBase.h"), interface_template)
        || !write_file(QDir(out_dir).filePath(class_name + ".h"),
                       _details::render_template(header_template, template_vars))
        || !write_file(QDir(out_dir).filePath(class_name + ".cpp"),
                       _details::render_template(source_template, template_vars))
        || !write_file(QDir(out_dir).filePath("CMakeLists.txt"),
                       _details::render_template(cmake_template, template_vars)))
    {
        return false;
    }

    const QString output_metadata_path = QDir(out_dir).filePath("metadata.json");
    if (QFileInfo::exists(output_metadata_path))
    {
        QFile::remove(output_metadata_path);
    }

    if (!QFile::copy(metadata_file_path, output_metadata_path))
    {
        m_last_error = "Failed to copy metadata.json to: " + output_metadata_path;
        return false;
    }

    append_log("Created: PluginInterfaceBase.h");
    append_log("Created: " + class_name + ".h");
    append_log("Created: " + class_name + ".cpp");
    append_log("Created: CMakeLists.txt");
    append_log("Created: metadata.json");
    return true;
}

bool PluginBuilderView::execute_build(bool clean_first)
{
    const QString source_dir = m_path_source->path();
    if (source_dir.isEmpty())
    {
        m_last_error = "Source Dir cannot be empty.";
        return false;
    }

    if (!QFileInfo(source_dir).isDir())
    {
        m_last_error = "Source directory not found: " + source_dir;
        return false;
    }

    append_log("Source directory: " + QDir::toNativeSeparators(source_dir));

    const QString cmake_exe =QDir::fromNativeSeparators(QString::fromUtf8(env_config::cmake_path));
    const QString qt_package_dir =
        QDir::fromNativeSeparators(QString::fromUtf8(env_config::qt_path));
    const QString project_root_dir =
        QDir::fromNativeSeparators(QString::fromUtf8(env_config::project_root_path));

    QString error_message;

    if (!QFileInfo(cmake_exe).isFile())
    {
        m_last_error = "CMake executable not found: " + cmake_exe;
        return false;
    }

    if (!QFileInfo(QDir(qt_package_dir).filePath("Qt5Config.cmake")).isFile())
    {
        m_last_error = "Qt5Config.cmake not found: " + qt_package_dir;
        return false;
    }

    if (!QDir(project_root_dir).exists())
    {
        m_last_error = "Project root path not found: " + project_root_dir;
        return false;
    }

    QDir qt_prefix_dir(qt_package_dir);
    if (!qt_prefix_dir.cdUp() || !qt_prefix_dir.cdUp() || !qt_prefix_dir.cdUp())
    {
        m_last_error = "Invalid qt_path: " + qt_package_dir;
        return false;
    }

    const QString qt_prefix_path =
        QDir::fromNativeSeparators(qt_prefix_dir.absolutePath());
    const QString template_dir =
        QDir(project_root_dir).filePath("templates/plugin");
    if (!QDir(template_dir).exists())
    {
        m_last_error = "Template directory not found: " + template_dir;
        return false;
    }

    const bool use_msvc_environment =
        qt_prefix_path.contains("msvc", Qt::CaseInsensitive);

    m_path_cmake->edit->setText(cmake_exe);
    m_path_qt->edit->setText(qt_package_dir);

    append_log("Configure source: configure_info.h");
    append_log("CMake executable: " + QDir::toNativeSeparators(cmake_exe));
    append_log("Qt5_DIR: " + QDir::toNativeSeparators(qt_package_dir));
    append_log("Project root: " + QDir::toNativeSeparators(project_root_dir));

    const QString metadata_file_path = QDir(source_dir).filePath("metadata.json");
    QJsonObject metadata;
    if (!_details::read_json_object(metadata_file_path, metadata, error_message))
    {
        m_last_error = error_message;
        return false;
    }

    const QString plugin_name = metadata.value("name").toString().trimmed();
    const QString plugin_version = metadata.value("version").toString().trimmed();
    const QString class_name = metadata.value("className").toString().trimmed();
    if (plugin_name.isEmpty() || plugin_version.isEmpty() || class_name.isEmpty())
    {
        m_last_error = "metadata.json is missing name, version or className.";
        return false;
    }

    append_log("Metadata file: " + QDir::toNativeSeparators(metadata_file_path));
    append_log(QString("Plugin: %1 (Version: %2, Class: %3)")
               .arg(plugin_name, plugin_version, class_name));

    m_out_dir = source_dir + "/build/" + class_name;
    const QString build_dir = m_out_dir + "/build";
    const QString built_plugin_path = build_dir + "/Release/" + class_name + ".dll";

    if (!QDir().mkpath(m_out_dir))
    {
        m_last_error = "Failed to create output directory: " + m_out_dir;
        return false;
    }

    if (clean_first && QDir(build_dir).exists() && !QDir(build_dir).removeRecursively())
    {
        m_last_error = "Failed to clean build directory: " + build_dir;
        return false;
    }

    if (!QDir().mkpath(build_dir))
    {
        m_last_error = "Failed to create build directory: " + build_dir;
        return false;
    }

    append_log("Output directory: " + QDir::toNativeSeparators(m_out_dir));
    append_log("Build directory: " + QDir::toNativeSeparators(build_dir));

    m_progress->setValue(10);
    if (!generate_plugin_files(m_out_dir, template_dir, metadata_file_path, metadata))
    {
        return false;
    }

    m_progress->setValue(20);
    if (!run_process(cmake_exe,
                     {
                         "-S", m_out_dir,
                         "-B", build_dir,
                         "-DCMAKE_PREFIX_PATH=" + qt_prefix_path,
                         "-DQt5_DIR=" + qt_package_dir,
                         "-DCMAKE_BUILD_TYPE=Release"
                     },
                     build_dir,
                     use_msvc_environment))
    {
        m_last_error = "CMake configure failed: " + m_last_error;
        return false;
    }

    m_progress->setValue(50);
    if (!run_process(cmake_exe,
                     {"--build", build_dir, "--config", "Release"},
                     build_dir,
                     use_msvc_environment))
    {
        m_last_error = "CMake build failed: " + m_last_error;
        return false;
    }

    if (!QFileInfo::exists(built_plugin_path))
    {
        m_last_error = "Built plugin DLL not found: " + built_plugin_path;
        return false;
    }

    const QString publish_dir =
        QDir(project_root_dir).filePath("plugins/" + plugin_name + "/" + plugin_version);
    const QString publish_path =
        QDir(publish_dir).filePath(plugin_name + "_" + plugin_version + ".dll");

    if (!QDir().mkpath(publish_dir))
    {
        m_last_error = "Failed to create plugin publish directory: " + publish_dir;
        return false;
    }

    if (QFileInfo::exists(publish_path) && !QFile::remove(publish_path))
    {
        m_last_error = "Failed to replace existing plugin DLL: " + publish_path;
        return false;
    }

    if (!QFile::copy(built_plugin_path, publish_path))
    {
        m_last_error = "Failed to publish plugin DLL to: " + publish_path;
        return false;
    }

    m_progress->setValue(100);
    append_log("Published plugin DLL: " + QDir::toNativeSeparators(publish_path));
    return true;
}

void PluginBuilderView::on_open_output_dir()
{
    if (!m_out_dir.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_out_dir));
    }
}
