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
#include "../mustache/mustache.hpp"

namespace _details
{
// Build tools on Windows may emit either UTF-8 or the active ANSI code page.
// Try UTF-8 first and keep it only when the byte stream round-trips cleanly.
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

// Metadata values are injected into generated C++ source code. Escape them so
// quotes, backslashes, and non-ASCII characters remain valid inside u8 literals.
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
}

class template_engine
{
public:
    static QString render(const QString &tpl, const QVariantMap &vars)
    {
        using namespace kainjow::mustache;
        data d;
        for (auto it = vars.constBegin(); it != vars.constEnd(); ++it)
        {
            QString v = it.value().toString();
            if (v.contains('\\')) v = QDir::fromNativeSeparators(v);
            d[ it.key().toStdString() ] = v.toStdString();
        }
        mustache tmpl{ tpl.toStdString() };
        return QString::fromStdString( tmpl.render(d) );
    }
};

PathSelector::PathSelector(const Mode m, QWidget *parent)
    : QWidget(parent), m_mode(m)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);

    m_edit = new QLineEdit(this);
    auto *select_button = new QPushButton("...", this);

    layout->addWidget(m_edit);
    layout->addWidget(select_button);

    connect(select_button, &QPushButton::clicked,this, [this]{
                const auto  res = (m_mode == Directory)
    ? QFileDialog::getExistingDirectory(this, "Select Directory")
    : QFileDialog::getOpenFileName(this, "Select File");
    if (!res.isEmpty())m_edit->setText(QDir::fromNativeSeparators(res));});
}


PluginBuilderView::PluginBuilderView(QWidget *parent)
    : QWidget(parent)
{
    setup_ui();
    setWindowTitle("PluginBuilder");
    resize(900, 650);
}

void PluginBuilderView::setup_ui()
{
    auto *main_layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    m_path_source = new PathSelector(PathSelector::Directory);
    m_path_cmake  = new PathSelector(PathSelector::File);
    m_path_qt     = new PathSelector(PathSelector::Directory);

    form->addRow("Source Directory:",   m_path_source);
    form->addRow("CMake Executable:", m_path_cmake);
    form->addRow("Qt Prefix (MSVC/GCC):", m_path_qt);

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

    connect(m_build,&QPushButton::clicked,this, [this]{
        m_log_edit->clear();
        m_progress->setValue(0);
        m_open_dir->setEnabled(false);

        if (execute_build(false))
        {
            m_open_dir->setEnabled(true);
            QMessageBox::information(this, "Success", "Build finished!");
            on_open_output_dir();
        }
        else
            QMessageBox::critical(this, "Error", m_last_error);
    });

    connect(m_clean_build, &QPushButton::clicked, this, [this]{
        m_log_edit->clear();
        m_progress->setValue(0);
        m_open_dir->setEnabled(false);

        if (execute_build(true))
        {
            m_open_dir->setEnabled(true);
            QMessageBox::information(this, "Success", "Clean rebuild finished!");
            on_open_output_dir();
        }
        else
            QMessageBox::critical(this, "Error", m_last_error);
    });
}


void PluginBuilderView::append_log(const QString &log)
{
    m_log_edit->appendPlainText(log);
}

void PluginBuilderView::write_file(const QString &path, const QString &content)
{
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
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
    }
}

bool PluginBuilderView::run_process(const QString &cmd,
                                const QStringList &args,
                                const QString &wd)
{
    // Log a shell-safe representation of the command so the user can copy it
    // from the UI and reproduce configure/build steps outside the application.
    auto quote_argument = [](const QString &value) {
        QString escaped = value;
        escaped.replace('"', "\\\"");
        if (escaped.contains(' ') || escaped.contains('\t')) {
            return "\"" + escaped + "\"";
        }
        return escaped;
    };

    QStringList formatted_args;
    for (const QString &arg : args) {
        formatted_args.append(quote_argument(arg));
    }

    QProcess p;
    p.setWorkingDirectory(wd);
    // Keep the working directory explicit because both configure and build use
    // generated paths under the temporary plugin output tree.
    append_log("Working directory: " + QDir::toNativeSeparators(wd));
    append_log(QString("Running: %1 %2")
               .arg(quote_argument(cmd), formatted_args.join(" ")));
    // Start the external tool and wait synchronously because later steps depend
    // on the previous command finishing successfully.
    p.start(QDir::fromNativeSeparators(cmd), args);
    if (!p.waitForFinished(60000)) {
        m_last_error = "Timeout";
        return false;
    }
    // Decode stdout/stderr before appending them so Chinese diagnostics from
    // CMake, MSBuild, and compiler tools remain readable in the log view.
    const QString standard_output =
        _details::decode_process_output(p.readAllStandardOutput());
    const QString standard_error =
        _details::decode_process_output(p.readAllStandardError());

    if (!standard_output.isEmpty()) {
        append_log(standard_output);
    }

    if (!standard_error.isEmpty()) {
        append_log(standard_error);
    }
    if (p.exitCode() != 0) {
        if (!standard_error.trimmed().isEmpty()) {
            m_last_error = standard_error.trimmed();
        } else if (!standard_output.trimmed().isEmpty()) {
            m_last_error = standard_output.trimmed();
        } else {
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

            if (process.waitForFinished(5000) && process.exitCode() == 0) {
                const QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
                if (!output.isEmpty()) {
                    const QStringList lines = output.split('\n', QString::SkipEmptyParts);
                    for (const QString &line : lines) {
                        const QString vcvars_path = try_candidate(line.trimmed());
                        if (!vcvars_path.isEmpty()) {
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

bool PluginBuilderView::run_process_with_msvc_environment(const QString &cmd,
                                                       const QStringList &args,
                                                       const QString &working_dir)
{
#ifdef Q_OS_WIN
    // Batch quoting is different from QProcess argument quoting. Build the
    // wrapper command line carefully so paths with spaces survive cmd.exe.
    auto quote_for_batch = [](const QString &value) {
        QString escaped = QDir::toNativeSeparators(value);
        escaped.replace('"', "\"\"");
        if (escaped.contains(' ') || escaped.contains('\t')) {
            return "\"" + escaped + "\"";
        }
        return escaped;
    };

    const QString vcvars_path = find_vcvars64();
    if (vcvars_path.isEmpty()) {
        m_last_error = "vcvars64.bat not found. Visual Studio build environment is unavailable.";
        return false;
    }

    QStringList formatted_args;
    for (const QString &arg : args) {
        formatted_args.append(quote_for_batch(arg));
    }

    const QString joined_args = formatted_args.join(" ");
    const QString command_line = joined_args.isEmpty()
        ? quote_for_batch(cmd)
        : quote_for_batch(cmd) + " " + joined_args;

    // Write a short wrapper script that loads vcvars64.bat and then runs the
    // real command. This keeps the generated plugin build independent from the
    // environment used to launch the main application.
    QTemporaryFile script_file(QDir::tempPath() + "/PluginBuilder-XXXXXX.cmd");
    if (!script_file.open()) {
        m_last_error = "Failed to create temporary MSVC environment script.";
        return false;
    }

    const QString script_content =
        "@echo off\r\n"
        "call " + quote_for_batch(vcvars_path) + " >nul\r\n"
        "if errorlevel 1 exit /b %errorlevel%\r\n"
        + command_line + "\r\n";

    script_file.write(script_content.toLocal8Bit());
    script_file.flush();
    const QString script_path = QDir::fromNativeSeparators(script_file.fileName());

    append_log("MSVC environment script: " + QDir::toNativeSeparators(vcvars_path));
    append_log("MSVC wrapper script: " + QDir::toNativeSeparators(script_path));
    append_log("MSVC wrapped command:");
    append_log(command_line);

    const QString comspec =
        QDir::fromNativeSeparators(qEnvironmentVariable("ComSpec",
                                                        "C:/Windows/System32/cmd.exe"));
    return run_process(comspec, {"/d", "/c", script_path}, working_dir);
#else
    return run_process(cmd, args, working_dir);
#endif
}

bool PluginBuilderView::execute_build(bool clean_first)
{
    // 1. Validate the three user inputs required to generate and build a plugin:
    //    source directory, CMake executable, and Qt installation path.
    if (m_path_source->path().isEmpty()) {
        m_last_error = "Source Dir cannot be empty.";
        return false;
    }

    if (m_path_cmake->path().isEmpty()) {
        m_last_error = "CMake Dir cannot be empty.";
        return false;
    }

    if (m_path_qt->path().isEmpty()) {
        m_last_error = "Qt Dir cannot be empty.";
        return false;
    }

    // 2. Normalize the text entered in the UI so all later file and process
    //    operations use the same path format.
    const QString source_dir = QDir::fromNativeSeparators(m_path_source->path());
    const QString cmake_exe = QDir::fromNativeSeparators(m_path_cmake->path());
    const QString qt_input_dir = QDir::fromNativeSeparators(m_path_qt->path());

    auto has_qt5_package = [](const QString &dir) -> bool {
        return QFileInfo::exists(dir + "/Qt5Config.cmake");
    };

    auto has_qt_prefix = [&](const QString &dir) -> bool {
        return has_qt5_package(dir + "/lib/cmake/Qt5");
    };

    QString qt_prefix_dir;
    QString qt_package_dir;

    // 3. Resolve the Qt path. The user may select the Qt prefix directory,
    //    the bin directory, or the lib/cmake/Qt5 package directory.
    if (has_qt_prefix(qt_input_dir)) {
        qt_prefix_dir = qt_input_dir;
        qt_package_dir = qt_input_dir + "/lib/cmake/Qt5";
    } else if (has_qt5_package(qt_input_dir)) {
        qt_package_dir = qt_input_dir;

        QDir prefix_dir(qt_input_dir);
        if (!prefix_dir.cdUp() || !prefix_dir.cdUp() || !prefix_dir.cdUp()) {
            m_last_error = "Invalid Qt package directory: " + qt_input_dir;
            return false;
        }

        qt_prefix_dir = QDir::fromNativeSeparators(prefix_dir.absolutePath());
    } else if (QFileInfo(qt_input_dir + "/qmake.exe").exists()
               || QFileInfo(qt_input_dir + "/qmake").exists()) {
        QDir prefix_dir(qt_input_dir);
        if (!prefix_dir.cdUp()) {
            m_last_error = "Invalid Qt bin directory: " + qt_input_dir;
            return false;
        }

        qt_prefix_dir = QDir::fromNativeSeparators(prefix_dir.absolutePath());
        qt_package_dir = qt_prefix_dir + "/lib/cmake/Qt5";
        if (!has_qt5_package(qt_package_dir)) {
            m_last_error = "Qt5Config.cmake not found under: " + qt_prefix_dir;
            return false;
        }
    } else {
        m_last_error =
            "Qt path must point to a Qt prefix, Qt bin directory, or lib/cmake/Qt5.";
        return false;
    }

    // 4. Read metadata.json from the selected source directory and validate
    //    the fields needed to generate the wrapper plugin project.
    QDir src_dir(source_dir);
    QFileInfoList metadata_files = src_dir.entryInfoList(
        QStringList() << "metadata.json",
        QDir::Files | QDir::NoDotAndDotDot,
        QDir::Name);

    if (metadata_files.isEmpty()) {
        m_last_error = "metadata.json not found in source directory.";
        return false;
    }

    QFile metadata_file(metadata_files.first().absoluteFilePath());
    if (!metadata_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_last_error = "Failed to open metadata.json";
        return false;
    }

    QJsonParseError parse_error{};
    const QJsonDocument doc = QJsonDocument::fromJson(metadata_file.readAll(), &parse_error);
    metadata_file.close();

    if (parse_error.error != QJsonParseError::NoError) {
        m_last_error = "Failed to parse metadata.json: " + parse_error.errorString();
        return false;
    }

    QJsonObject metadata = doc.object();

    QStringList required_keys = {"name", "version", "className"};
    for (const QString &key : required_keys) {
        if (!metadata.contains(key) || metadata[key].toString().isEmpty()) {
            m_last_error = "Missing required key in metadata.json: " + key;
            return false;
        }
    }

    QString plugin_name = metadata["name"].toString();
    QString plugin_version = metadata["version"].toString();
    QString class_name = metadata["className"].toString();
    QString display_name = metadata["displayName"].toString(plugin_name);
    QString description = metadata["description"].toString("No description");

    append_log("Source directory: " + source_dir);
    append_log("CMake executable: " + cmake_exe);
    append_log("Qt input path: " + qt_input_dir);
    append_log("Metadata file: " + metadata_files.first().absoluteFilePath());
    append_log(QString("Plugin: %1 (Version: %2, Class: %3)").arg(plugin_name, plugin_version, class_name));
    append_log("Qt prefix: " + qt_prefix_dir);
    append_log("Qt5_DIR: " + qt_package_dir);

    const bool use_msvc_environment =
        qt_prefix_dir.contains("msvc", Qt::CaseInsensitive);
    append_log(QString("MSVC environment required: %1")
               .arg(use_msvc_environment ? "yes" : "no"));

    // 5. Prepare the generated plugin output directory and the nested build
    //    directory that CMake will use for this standalone plugin project.
    m_out_dir = source_dir + "/build/" + class_name;
    QDir out_dir(m_out_dir);
    if (!out_dir.exists()) {
        // Create the generated plugin root directory first. All generated
        // source files and the nested CMake build tree live under this path.
        if (!out_dir.mkpath(".")) {
            m_last_error = "Failed to create output directory: " + m_out_dir;
            return false;
        }
    }

    QString build_dir = m_out_dir + "/build";

    // Remove the previous generated build tree when a clean rebuild is
    // requested so stale CMake cache files do not affect the new build.

    // 清理构建目录（如果需要）
    // 6. Remove the previous generated build tree when the user requested
    //    a clean rebuild so CMake configures from scratch.
    if (clean_first) {
        append_log("Cleaning build directory...");
        QDir buildDir(build_dir);
        if (buildDir.exists()) {
            if (!buildDir.removeRecursively()) {
                m_last_error = "Failed to clean build directory: " + build_dir;
                return false;
            }
        }
        append_log("Build directory cleaned.");
    }

    // Ensure the nested build tree exists before CMake writes cache files.
    QDir(build_dir).mkpath(".");

    append_log("Output directory: " + m_out_dir);
    append_log("Build directory: " + build_dir);

    // 7. Generate the interface header, plugin class source files, copied
    //    metadata, and CMakeLists.txt used for the plugin build.
    QString base_header_tpl = R"(#ifndef PLUGININTERFACEBASE_H
#define PLUGININTERFACEBASE_H

#include <QtPlugin>
#include <QString>
#include <QStringList>
#include <QWidget>

class PluginInterfaceBase
{
public:
    virtual ~PluginInterfaceBase() = default;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual QString version() const = 0;
    [[nodiscard]] virtual QStringList history() const = 0;

    virtual QWidget* create_widget(QWidget *parent = nullptr) = 0;
};

#define PluginInterfaceBase_iid "org.example.PluginInterface/1.0"
Q_DECLARE_INTERFACE(PluginInterfaceBase, PluginInterfaceBase_iid)

#endif
)";
    write_file(m_out_dir + "/PluginInterfaceBase.h", base_header_tpl);
    append_log("Created: PluginInterfaceBase.h");

    QString header_tpl = R"(#ifndef {{CLASS_NAME}}_H
#define {{CLASS_NAME}}_H

#include "PluginInterfaceBase.h"
#include <QWidget>
#include <QString>
#include <QStringList>

class {{CLASS_NAME}} : public QWidget, public PluginInterfaceBase
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterfaceBase_iid FILE "metadata.json")
    Q_INTERFACES(PluginInterfaceBase)

public:
    explicit {{CLASS_NAME}}(QWidget *parent = nullptr);
    ~{{CLASS_NAME}}() override = default;

    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString version() const override;
    [[nodiscard]] QStringList history() const override;

    QWidget* create_widget(QWidget *parent = nullptr) override;

private:
    void setup_ui();
};

#endif // {{CLASS_NAME}}_H
)";

    QVariantMap header_vars;
    header_vars["CLASS_NAME"] = class_name;
    QString header_content = template_engine::render(header_tpl, header_vars);
    write_file(m_out_dir + "/" + class_name + ".h", header_content);
    append_log("Created: " + class_name + ".h");

    QString source_tpl = R"(#include "{{CLASS_NAME}}.h"
#include <QVBoxLayout>
#include <QLabel>

{{CLASS_NAME}}::{{CLASS_NAME}}(QWidget *parent)
    : QWidget(parent)
{
    setup_ui();
}

QString {{CLASS_NAME}}::name() const
{
    return "{{PLUGIN_NAME}}";
}

QString {{CLASS_NAME}}::version() const
{
    return "{{VERSION}}";
}

QStringList {{CLASS_NAME}}::history() const
{
    return QStringList() << "{{VERSION}}";
}

QWidget* {{CLASS_NAME}}::create_widget(QWidget *parent)
{
    return new {{CLASS_NAME}}(parent);
}

void {{CLASS_NAME}}::setup_ui()
{
    auto *layout = new QVBoxLayout(this);
    const QString label_text = QString::fromUtf8(u8"{{LABEL_TEXT}}");
    auto *label = new QLabel(label_text, this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}
)";

    QVariantMap source_vars;
    source_vars["CLASS_NAME"] = class_name;
    source_vars["PLUGIN_NAME"] = plugin_name;
    source_vars["VERSION"] = plugin_version;
    source_vars["LABEL_TEXT"] =
        _details::escape_cpp_u8_string_literal(display_name + " - " + description);
    // Render the plugin implementation after metadata validation so the
    // generated widget source always reflects the current metadata.json.
    QString source_content = template_engine::render(source_tpl, source_vars);
    write_file(m_out_dir + "/" + class_name + ".cpp", source_content);
    append_log("Created: " + class_name + ".cpp");

    // Copy metadata.json beside the generated source because Qt reads plugin
    // metadata from the generated project directory during the build.
    QFile::copy(metadata_files.first().absoluteFilePath(), m_out_dir + "/metadata.json");
    append_log("Created: metadata.json");

    QString cmake_tpl = R"(cmake_minimum_required(VERSION 3.20)

# 设置 Qt 路径（必须在 project 之前）
list(APPEND CMAKE_PREFIX_PATH "{{qt_prefix_path}}")

# Qt path can be provided by the builder or overridden by configure arguments.
list(APPEND CMAKE_PREFIX_PATH "{{qt_prefix_path}}")

project({{CLASS_NAME}} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

add_library({{CLASS_NAME}} MODULE
    {{CLASS_NAME}}.h
    {{CLASS_NAME}}.cpp
    metadata.json
)

target_link_libraries({{CLASS_NAME}} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_compile_definitions({{CLASS_NAME}} PRIVATE QT_PLUGIN)
)";

    QVariantMap cmake_vars;
    cmake_vars["CLASS_NAME"] = class_name;
    cmake_vars["qt_prefix_path"] = qt_prefix_dir;
    QString cmake_content = template_engine::render(cmake_tpl, cmake_vars);
    write_file(m_out_dir + "/CMakeLists.txt", cmake_content);
    append_log("Created: CMakeLists.txt");

    // 8. Build the exact CMake configure command. When the selected Qt kit is
    //    MSVC-based, run it through vcvars64.bat so Windows SDK libraries such
    //    as kernel32.lib are available during CMake compiler detection.
    append_log("Starting CMake build...");
    m_progress->setValue(20);

    // CMake 配置（使用 Ninja 生成器）
    // Compose the configure arguments that will be passed to CMake.
    // Configure the generated plugin project. The source directory is the
    // generated wrapper root and the build directory is the nested build tree.
    const QStringList configure_args{
        "-S", m_out_dir,
        "-B", build_dir,
        "-DCMAKE_PREFIX_PATH=" + qt_prefix_dir,
        "-DQt5_DIR=" + qt_package_dir,
        "-DCMAKE_BUILD_TYPE=Release"
    };

    append_log("CMake configure command:");
    // Run CMake configure through QProcess. When the chosen Qt kit is MSVC-
    // based, wrap the process with vcvars64.bat so compiler and SDK variables
    // are present exactly as they would be in a Developer Command Prompt.
    const bool configure_ok = use_msvc_environment
        ? run_process_with_msvc_environment(cmake_exe, configure_args, build_dir)
        : run_process(cmake_exe, configure_args, build_dir);
    if (!configure_ok) {
        m_last_error = "CMake configure failed: " + m_last_error;
        return false;
    }
    m_progress->setValue(50);

    // 9. After configure succeeds, run the build command through the same
    //    environment wrapper so the linker can resolve Windows SDK libraries.
    const QStringList build_args{"--build", build_dir, "--config", "Release"};
    append_log("CMake build command:");
    // Run the actual compilation step through QProcess so compiler output is
    // streamed back into the log view and surfaced in m_last_error on failure.
    const bool build_ok = use_msvc_environment
        ? run_process_with_msvc_environment(cmake_exe, build_args, build_dir)
        : run_process(cmake_exe, build_args, build_dir);
    if (!build_ok) {
        m_last_error = "CMake build failed: " + m_last_error;
        return false;
    }
    m_progress->setValue(100);

    // 输出调试信息：当前工作目录、源码目录
    append_log("Current working directory: " + QDir::toNativeSeparators(QDir::currentPath()));
    append_log("Source directory: " + QDir::toNativeSeparators(source_dir));

    append_log("Build completed successfully!");

// 先找到构建产物 DLL
const QString built_plugin_path = build_dir + "/Release/" + class_name + ".dll";
append_log("Build DLL path: " + QDir::toNativeSeparators(built_plugin_path));

if (!QFileInfo::exists(built_plugin_path))
{
    m_last_error = "Built plugin DLL not found: " + built_plugin_path;
    append_log("Error: " + QDir::toNativeSeparators(m_last_error));
    return false;
}

// 不再从 source_dir 查找项目根目录，
// 而是从当前程序所在目录开始，逐级向上查找 project_root.txt
const QString app_dir = QCoreApplication::applicationDirPath();
append_log("Application directory: " + QDir::toNativeSeparators(app_dir));
append_log("Source directory: " + QDir::toNativeSeparators(source_dir));

QDir project_root_dir(app_dir);
bool found_project_root = false;

while (true)
{
    const QString marker_file_path = project_root_dir.filePath("project_root.txt");
    append_log("Checking project root marker: " + QDir::toNativeSeparators(marker_file_path));

    if (QFileInfo::exists(marker_file_path) && QFileInfo(marker_file_path).isFile())
    {
        found_project_root = true;
        append_log("Current project root found: "+ QDir::toNativeSeparators(project_root_dir.absolutePath()));
        break;
    }

    if (!project_root_dir.cdUp())
    {
        break;
    }
}

if (!found_project_root)
{
    m_last_error = "Failed to locate current project root from application directory: " + app_dir;
    append_log("Error: " + QDir::toNativeSeparators(m_last_error));
    return false;
}

// 最终输出目录：当前项目根目录/plugins/插件名/版本号
const QString publish_dir =
    project_root_dir.filePath("plugins/" + plugin_name + "/" + plugin_version);
append_log("Publish directory: " + QDir::toNativeSeparators(publish_dir));

if (!QDir().mkpath(publish_dir))
{
    m_last_error = "Failed to create plugin publish directory: " + publish_dir;
    append_log("Error: " + QDir::toNativeSeparators(m_last_error));
    return false;
}

// 最终输出文件：当前项目根目录/plugins/插件名/版本号/插件名_版本号.dll
const QString publish_file_name = plugin_name + "_" + plugin_version + ".dll";
const QString publish_path = QDir(publish_dir).filePath(publish_file_name);

append_log("Publish file name: " + publish_file_name);
append_log("Publish file path: " + QDir::toNativeSeparators(publish_path));

// 如果目标文件已存在，先删除旧文件
if (QFileInfo::exists(publish_path))
{
    append_log("Existing publish file found, removing: "
               + QDir::toNativeSeparators(publish_path));

    if (!QFile::remove(publish_path))
    {
        m_last_error = "Failed to replace existing plugin DLL: " + publish_path;
        append_log("Error: " + QDir::toNativeSeparators(m_last_error));
        return false;
    }
}

// 复制 DLL 到目标目录
if (!QFile::copy(built_plugin_path, publish_path))
{
    m_last_error = "Failed to publish plugin DLL to: " + publish_path;
    append_log("Error: " + QDir::toNativeSeparators(m_last_error));
    return false;
}

append_log("Published plugin DLL: " + QDir::toNativeSeparators(publish_path));
return true;
}


void PluginBuilderView::on_open_output_dir()
{
    if (!m_out_dir.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_out_dir));
    }
}

#include "PluginBuilder.moc"
