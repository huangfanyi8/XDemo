#ifndef PLUGINBUILDER_H
#define PLUGINBUILDER_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QVariantMap>
#include <QJsonObject>

struct PathSelector
    :QWidget
{
    Q_OBJECT
public:
    explicit PathSelector(QWidget *parent = nullptr);
    [[nodiscard]] QString path() const{return QDir::fromNativeSeparators(edit->text().trimmed());}

    template<class... PathSelectors, bool... directory>
    static void _connect_signals(std::integer_sequence<bool, directory...>, PathSelectors*... selectors)
    {
        (
            QObject::connect(
                selectors->browse,
                &QPushButton::clicked,
                selectors,
                [selector = selectors, is_directory = directory]()
                {
                    const QString res = is_directory
                        ? QFileDialog::getExistingDirectory(selector, "Select Directory")
                        : QFileDialog::getOpenFileName(selector, "Select File");

                    if (!res.isEmpty())
                    {
                        selector->edit->setText(QDir::fromNativeSeparators(res));
                    }
                }),
            ...);
    }

    QLineEdit *edit;
    QPushButton*browse;
};

class PluginBuilderView : public QWidget
{
    Q_OBJECT
public:
    explicit PluginBuilderView(QWidget *parent = nullptr);

private slots:
    /**
     * @brief 在系统文件管理器中打开当前输出目录。
     */
    void on_open_output_dir();

private:
    /**
     * @brief 初始化界面控件与布局。
     */
    void setup_ui();

    /**
     * @brief 执行插件构建完整流程。
     * @param clean_first 为 true 时先清理旧的构建目录。
     * @return 构建与发布全部成功时返回 true，否则返回 false。
     */
    bool execute_build(bool clean_first = false);

    /**
     * @brief 读取模板并生成构建所需代码文件。
     * @param out_dir 输出目录。
     * @param template_dir 模板目录。
     * @param metadata_file_path 元数据文件路径。
     * @param metadata 元数据对象。
     * @return 代码生成成功时返回 true，否则返回 false。
     */
    bool generate_plugin_files(const QString &out_dir,
                               const QString &template_dir,
                               const QString &metadata_file_path,
                               const QJsonObject &metadata);

    /**
     * @brief 运行外部进程，并在需要时自动加载 MSVC 环境。
     * @param cmd 可执行文件路径。
     * @param args 命令行参数列表。
     * @param working_dir 进程工作目录。
     * @param use_msvc_environment 为 true 时先加载 MSVC 环境。
     * @return 构建成功时返回 true，否则返回 false。
     */
    bool run_process(const QString &cmd,
                     const QStringList &args,
                     const QString &working_dir,
                     bool use_msvc_environment = false);

    /**
     * @brief 查找 MSVC 构建所需的 vcvars64.bat。
     * @return 找到时返回绝对路径，否则返回空字符串。
     */
    QString find_vcvars64() const;

    /**
     * @brief 向文件写入文本内容。
     * @param path 目标文件路径。
     * @param content 要写入的内容。
     * @return 写入成功时返回 true，否则返回 false。
     */
    bool write_file(const QString &path, const QString &content);

    /**
     * @brief 向日志窗口追加一行文本。
     * @param log 日志内容。
     */
    void append_log(const QString &log);


    void _connect_signals()
    {
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

        PathSelector::_connect_signals(std::integer_sequence<bool,true>{}, m_path_source);
    }

    QHBoxLayout *m_layout;
    PathSelector *m_path_source;
    PathSelector *m_path_cmake;
    PathSelector *m_path_qt;
    QPushButton   *m_build;
    QPushButton   *m_clean_build;
    QPushButton   *m_open_dir;
    QProgressBar *m_progress;
    QPlainTextEdit* m_log_edit;

    QString m_last_error;
    QString m_out_dir;
};

#endif // PLUGINBUILDER_H
