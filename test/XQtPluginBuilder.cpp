#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

struct Metadata
{
    QString m_name;
    QString m_version;
    QString m_class_name;
};

class MainWindow : public QWidget
{
public:
    MainWindow()
    {
        setWindowTitle(QStringLiteral("Qt 插件生成器 Demo"));
        resize(860, 620);

        _source_dir_edit = new QLineEdit(this);
        _cmake_path_edit = new QLineEdit(this);
        _qt_prefix_edit = new QLineEdit(this);
        _generator_edit = new QLineEdit(this);
        _platform_edit = new QLineEdit(this);

        _browse_source_button = new QPushButton(QStringLiteral("浏览"), this);
        _browse_cmake_button = new QPushButton(QStringLiteral("浏览"), this);
        _browse_qt_button = new QPushButton(QStringLiteral("浏览"), this);
        _build_button = new QPushButton(QStringLiteral("生成插件"), this);

        _progress_bar = new QProgressBar(this);
        _progress_bar->setRange(0, 100);
        _progress_bar->setValue(0);

        _log_edit = new QPlainTextEdit(this);
        _log_edit->setReadOnly(true);

        // 这里给你放上示例默认值，按你自己的环境改
        _cmake_path_edit->setText(QStringLiteral("D:/CLion/CLion/bin/cmake/win/x64/bin/cmake.exe"));
        _qt_prefix_edit->setText(QStringLiteral("C:/Qt/Qt5.12.9/5.12.9/msvc2017_64"));
        _generator_edit->setText(QStringLiteral("Visual Studio 17 2022"));
        _platform_edit->setText(QStringLiteral("x64"));

        auto *form_layout = new QFormLayout;

        auto *source_layout = new QHBoxLayout;
        source_layout->addWidget(_source_dir_edit);
        source_layout->addWidget(_browse_source_button);

        auto *cmake_layout = new QHBoxLayout;
        cmake_layout->addWidget(_cmake_path_edit);
        cmake_layout->addWidget(_browse_cmake_button);

        auto *qt_layout = new QHBoxLayout;
        qt_layout->addWidget(_qt_prefix_edit);
        qt_layout->addWidget(_browse_qt_button);

        form_layout->addRow(QStringLiteral("源码目录:"), source_layout);
        form_layout->addRow(QStringLiteral("CMake 路径:"), cmake_layout);
        form_layout->addRow(QStringLiteral("Qt CMAKE_PREFIX_PATH:"), qt_layout);
        form_layout->addRow(QStringLiteral("生成器:"), _generator_edit);
        form_layout->addRow(QStringLiteral("平台:"), _platform_edit);

        auto *main_layout = new QVBoxLayout(this);
        main_layout->addLayout(form_layout);
        main_layout->addWidget(_build_button);
        main_layout->addWidget(new QLabel(QStringLiteral("构建进度:"), this));
        main_layout->addWidget(_progress_bar);
        main_layout->addWidget(new QLabel(QStringLiteral("构建日志:"), this));
        main_layout->addWidget(_log_edit);

        connect(_browse_source_button, &QPushButton::clicked, this, [this]() {
            const QString dir = QFileDialog::getExistingDirectory(
                this,
                QStringLiteral("选择控件源码目录"));

            if (!dir.isEmpty())
            {
                _source_dir_edit->setText(dir);
            }
        });

        connect(_browse_cmake_button, &QPushButton::clicked, this, [this]() {
            const QString file = QFileDialog::getOpenFileName(
                this,
                QStringLiteral("选择 cmake.exe"),
                QString(),
                QStringLiteral("Executable (*.exe);;All Files (*)"));

            if (!file.isEmpty())
            {
                _cmake_path_edit->setText(QDir::fromNativeSeparators(file));
            }
        });

        connect(_browse_qt_button, &QPushButton::clicked, this, [this]() {
            const QString dir = QFileDialog::getExistingDirectory(
                this,
                QStringLiteral("选择 Qt 前缀目录"));

            if (!dir.isEmpty())
            {
                _qt_prefix_edit->setText(QDir::fromNativeSeparators(dir));
            }
        });

        connect(_build_button, &QPushButton::clicked, this, [this]() {
            _start_build();
        });
    }

private:
    void _append_log(const QString &text)
    {
        const QString time_text =
            QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
        _log_edit->appendPlainText(QStringLiteral("[%1] %2").arg(time_text, text));
    }

    void _set_progress(int value, const QString &message)
    {
        _progress_bar->setValue(value);
        _append_log(message);
        QCoreApplication::processEvents();
    }

    bool _write_text_file(const QString &file_path,
                          const QString &content,
                          QString *error_message)
    {
        QFile file(file_path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("无法写入文件:\n%1").arg(file_path);
            }
            return false;
        }

        QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        stream.setCodec("UTF-8");
#endif
        stream << content;
        file.close();
        return true;
    }

    bool _load_metadata(const QString &metadata_file_path,
                        Metadata *metadata,
                        QString *error_message)
    {
        if (metadata == nullptr)
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("metadata 指针为空。");
            }
            return false;
        }

        QFileInfo file_info(metadata_file_path);
        if (!file_info.exists() || !file_info.isFile())
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("metadata.json 不存在:\n%1")
                                     .arg(metadata_file_path);
            }
            return false;
        }

        QFile file(metadata_file_path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("无法打开 metadata.json:\n%1")
                                     .arg(metadata_file_path);
            }
            return false;
        }

        const QByteArray raw_data = file.readAll();
        file.close();

        QJsonParseError parse_error;
        const QJsonDocument json_document =
            QJsonDocument::fromJson(raw_data, &parse_error);

        if (parse_error.error != QJsonParseError::NoError || !json_document.isObject())
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("metadata.json 解析失败:\n%1")
                                     .arg(parse_error.errorString());
            }
            return false;
        }

        const QJsonObject object = json_document.object();

        metadata->m_name = object.value(QStringLiteral("name")).toString().trimmed();
        metadata->m_version = object.value(QStringLiteral("version")).toString().trimmed();
        metadata->m_class_name = object.value(QStringLiteral("className")).toString().trimmed();

        if (metadata->m_name.isEmpty())
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("metadata.json 缺少 name。");
            }
            return false;
        }

        if (metadata->m_version.isEmpty())
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("metadata.json 缺少 version。");
            }
            return false;
        }

        if (metadata->m_class_name.isEmpty())
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("metadata.json 缺少 className。");
            }
            return false;
        }

        return true;
    }

    bool _find_header_and_source(const QString &source_directory,
                                 QString *header_file_path,
                                 QString *source_file_path,
                                 QString *error_message)
    {
        QDir dir(source_directory);

        const QFileInfoList header_files =
            dir.entryInfoList(QStringList() << QStringLiteral("*.h") << QStringLiteral("*.hpp"),
                              QDir::Files | QDir::NoDotAndDotDot);

        const QFileInfoList source_files =
            dir.entryInfoList(QStringList() << QStringLiteral("*.cpp") << QStringLiteral("*.cc"),
                              QDir::Files | QDir::NoDotAndDotDot);

        if (header_files.size() != 1)
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("源码目录中必须且只能有 1 个头文件。");
            }
            return false;
        }

        if (source_files.size() != 1)
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("源码目录中必须且只能有 1 个源文件。");
            }
            return false;
        }

        if (header_file_path != nullptr)
        {
            *header_file_path = header_files.first().absoluteFilePath();
        }

        if (source_file_path != nullptr)
        {
            *source_file_path = source_files.first().absoluteFilePath();
        }

        return true;
    }

    bool _run_process(const QString &program,
                      const QStringList &arguments,
                      const QString &working_directory,
                      QString *error_message)
    {
        QProcess process;
        process.setProgram(program);
        process.setArguments(arguments);
        process.setWorkingDirectory(working_directory);
        process.setProcessChannelMode(QProcess::SeparateChannels);
        process.start();

        if (!process.waitForStarted())
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("无法启动进程:\n%1").arg(program);
            }
            return false;
        }

        while (!process.waitForFinished(100))
        {
            const QString out_text = QString::fromLocal8Bit(process.readAllStandardOutput());
            const QString err_text = QString::fromLocal8Bit(process.readAllStandardError());

            if (!out_text.trimmed().isEmpty())
            {
                _append_log(out_text.trimmed());
            }

            if (!err_text.trimmed().isEmpty())
            {
                _append_log(err_text.trimmed());
            }

            QCoreApplication::processEvents();
        }

        const QString final_out = QString::fromLocal8Bit(process.readAllStandardOutput());
        const QString final_err = QString::fromLocal8Bit(process.readAllStandardError());

        if (!final_out.trimmed().isEmpty())
        {
            _append_log(final_out.trimmed());
        }

        if (!final_err.trimmed().isEmpty())
        {
            _append_log(final_err.trimmed());
        }

        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        {
            if (error_message != nullptr)
            {
                *error_message = QStringLiteral("进程执行失败:\n%1\n\n参数:\n%2")
                                     .arg(program, arguments.join(QStringLiteral(" ")));
            }
            return false;
        }

        return true;
    }

    bool _build_plugin(QString *error_message)
    {
        const QString source_directory = _source_dir_edit->text().trimmed();
        const QString cmake_program = _cmake_path_edit->text().trimmed();
        const QString qt_cmake_prefix = _qt_prefix_edit->text().trimmed();
        const QString generator = _generator_edit->text().trimmed();
        const QString platform = _platform_edit->text().trimmed();

        if (source_directory.isEmpty())
        {
            *error_message = QStringLiteral("请先选择源码目录。");
            return false;
        }

        if (cmake_program.isEmpty())
        {
            *error_message = QStringLiteral("请先填写 CMake 路径。");
            return false;
        }

        if (qt_cmake_prefix.isEmpty())
        {
            *error_message = QStringLiteral("请先填写 Qt CMAKE_PREFIX_PATH。");
            return false;
        }

        _set_progress(5, QStringLiteral("开始检查输入参数。"));

        Metadata metadata;
        QString header_file_path;
        QString source_file_path;

        const QString metadata_file_path =
            QDir(source_directory).filePath(QStringLiteral("metadata.json"));

        _set_progress(15, QStringLiteral("读取 metadata.json。"));
        if (!_load_metadata(metadata_file_path, &metadata, error_message))
        {
            return false;
        }

        _set_progress(25, QStringLiteral("扫描头文件和源文件。"));
        if (!_find_header_and_source(source_directory,
                                     &header_file_path,
                                     &source_file_path,
                                     error_message))
        {
            return false;
        }

        const QString plugin_name = metadata.m_name + metadata.m_version;

        // 输出目录改成 plugins/name
        const QString plugins_root_directory =
            QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("plugins"));
        const QString plugin_output_directory =
            QDir(plugins_root_directory).filePath(metadata.m_name);

        const QString temp_root =
            QDir::temp().filePath(QStringLiteral("plugin_build_%1").arg(plugin_name));
        const QString build_directory =
            QDir(temp_root).filePath(QStringLiteral("build"));

        QDir temp_dir(temp_root);
        if (temp_dir.exists())
        {
            temp_dir.removeRecursively();
        }

        _set_progress(35, QStringLiteral("创建临时构建目录。"));

        if (!QDir().mkpath(temp_root))
        {
            *error_message = QStringLiteral("无法创建临时目录:\n%1").arg(temp_root);
            return false;
        }

        QDir().mkpath(build_directory);
        QDir().mkpath(plugin_output_directory);

        const QString interface_header_path =
            QDir(temp_root).filePath(QStringLiteral("IRuntimeComponentPlugin.h"));
        const QString wrapper_header_path =
            QDir(temp_root).filePath(QStringLiteral("GeneratedWrapper.h"));
        const QString wrapper_cpp_path =
            QDir(temp_root).filePath(QStringLiteral("GeneratedWrapper.cpp"));
        const QString cmake_lists_path =
            QDir(temp_root).filePath(QStringLiteral("CMakeLists.txt"));

        // 固定插件接口
        const QString interface_header_content =
            QStringLiteral(
R"(#pragma once

#include <QtPlugin>
#include <QString>
#include <QWidget>

/**
 * @brief 平台固定插件接口
 */
class IRuntimeComponentPlugin
{
public:
    virtual ~IRuntimeComponentPlugin() = default;

    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QWidget* create_widget(QWidget* parent = nullptr) = 0;
};

#define IRuntimeComponentPlugin_iid "com.lowcode.runtimecomponent"
Q_DECLARE_INTERFACE(IRuntimeComponentPlugin, IRuntimeComponentPlugin_iid)
)");

        // 自动生成包装类头文件
        const QString wrapper_header_content =
            QStringLiteral(
R"(#pragma once

#include <QObject>
#include <QtPlugin>

#include "IRuntimeComponentPlugin.h"

/**
 * @brief 自动生成的插件包装类
 */
class GeneratedWrapper : public QObject, public IRuntimeComponentPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IRuntimeComponentPlugin_iid)
    Q_INTERFACES(IRuntimeComponentPlugin)

public:
    QString name() const override;
    QString version() const override;
    QWidget* create_widget(QWidget* parent = nullptr) override;
};
)");

        const QString widget_header_name = QFileInfo(header_file_path).fileName();

        // 自动生成包装类 cpp
        const QString wrapper_cpp_content =
            QStringLiteral(
R"(#include "GeneratedWrapper.h"
#include "%1"

QString GeneratedWrapper::name() const
{
    return QStringLiteral("%2");
}

QString GeneratedWrapper::version() const
{
    return QStringLiteral("%3");
}

QWidget* GeneratedWrapper::create_widget(QWidget* parent)
{
    return new %4(parent);
}
)")
                .arg(widget_header_name,
                     metadata.m_name,
                     metadata.m_version,
                     metadata.m_class_name);

        // 临时插件工程
        const QString cmake_lists_content =
            QStringLiteral(
R"(cmake_minimum_required(VERSION 3.16)

project(%1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

set(CMAKE_PREFIX_PATH "%2")

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

add_library(%1 SHARED
    "%3"
    "%4"
    "${CMAKE_CURRENT_SOURCE_DIR}/IRuntimeComponentPlugin.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/GeneratedWrapper.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/GeneratedWrapper.cpp"
)

target_include_directories(%1
    PRIVATE
        "%5"
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(%1
    PRIVATE
        Qt5::Core
        Qt5::Widgets
)

set_target_properties(%1 PROPERTIES
    OUTPUT_NAME "%1"
    RUNTIME_OUTPUT_DIRECTORY "%6"
    LIBRARY_OUTPUT_DIRECTORY "%6"
    ARCHIVE_OUTPUT_DIRECTORY "%6"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "%6"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "%6"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "%6"
)
)")
                .arg(plugin_name,
                     QDir::fromNativeSeparators(qt_cmake_prefix),
                     QDir::fromNativeSeparators(source_file_path),
                     QDir::fromNativeSeparators(header_file_path),
                     QDir::fromNativeSeparators(source_directory),
                     QDir::fromNativeSeparators(plugin_output_directory));

        _set_progress(45, QStringLiteral("生成插件接口和包装代码。"));

        if (!_write_text_file(interface_header_path, interface_header_content, error_message))
        {
            return false;
        }

        if (!_write_text_file(wrapper_header_path, wrapper_header_content, error_message))
        {
            return false;
        }

        if (!_write_text_file(wrapper_cpp_path, wrapper_cpp_content, error_message))
        {
            return false;
        }

        if (!_write_text_file(cmake_lists_path, cmake_lists_content, error_message))
        {
            return false;
        }

        _set_progress(60, QStringLiteral("开始执行 CMake 配置。"));

        QStringList configure_args;
        configure_args << QStringLiteral("-S") << temp_root
                       << QStringLiteral("-B") << build_directory;

        if (!generator.isEmpty())
        {
            configure_args << QStringLiteral("-G") << generator;
        }

        if (!platform.isEmpty())
        {
            configure_args << QStringLiteral("-A") << platform;
        }

        if (!_run_process(cmake_program, configure_args, temp_root, error_message))
        {
            return false;
        }

        _set_progress(80, QStringLiteral("开始编译插件。"));

        QStringList build_args;
        build_args << QStringLiteral("--build") << build_directory
                   << QStringLiteral("--config") << QStringLiteral("Release");

        if (!_run_process(cmake_program, build_args, temp_root, error_message))
        {
            return false;
        }

#if defined(Q_OS_WIN)
        const QString plugin_output_path =
            QDir(plugin_output_directory).filePath(plugin_name + QStringLiteral(".dll"));
#elif defined(Q_OS_MAC)
        const QString plugin_output_path =
            QDir(plugin_output_directory).filePath(QStringLiteral("lib") + plugin_name + QStringLiteral(".dylib"));
#else
        const QString plugin_output_path =
            QDir(plugin_output_directory).filePath(QStringLiteral("lib") + plugin_name + QStringLiteral(".so"));
#endif

        if (!QFileInfo::exists(plugin_output_path))
        {
            *error_message = QStringLiteral("构建完成，但未找到插件文件:\n%1")
                                 .arg(plugin_output_path);
            return false;
        }

        _set_progress(100, QStringLiteral("插件生成完成。"));
        _append_log(QStringLiteral("输出文件: %1").arg(plugin_output_path));
        return true;
    }

    //build->
    void _start_build()
    {
        _build_button->setEnabled(false);
        _progress_bar->setValue(0);
        _log_edit->clear();

        QString error_message;
        const bool ok = _build_plugin(&error_message);

        _build_button->setEnabled(true);

        if (!ok)
        {
            QMessageBox::critical(this,
                                  QStringLiteral("生成失败"),
                                  error_message);
            return;
        }

        QMessageBox::information(this,
                                 QStringLiteral("生成成功"),
                                 QStringLiteral("插件已经生成完成。"));
    }

private:
    QLineEdit *_source_dir_edit = nullptr;
    QLineEdit *_cmake_path_edit = nullptr;
    QLineEdit *_qt_prefix_edit = nullptr;
    QLineEdit *_generator_edit = nullptr;
    QLineEdit *_platform_edit = nullptr;

    QPushButton *_browse_source_button = nullptr;
    QPushButton *_browse_cmake_button = nullptr;
    QPushButton *_browse_qt_button = nullptr;
    QPushButton *_build_button = nullptr;

    QProgressBar *_progress_bar = nullptr;
    QPlainTextEdit *_log_edit = nullptr;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}