/**
* @file PluginBuilderPalatte.h
 * @brief 插件面板类，用于构建插件并进行版本管理
 * @author REER Wrj_Qt5LowCode Team
 * @date 2025
 */

#pragma once

#include<QWidget>

struct SourcePathSelector :  QWidget
{
    Q_OBJECT

    public:
    explicit SourcePathSelector(QWidget *parent = nullptr);

    bool  validate() const { return !path().isEmpty(); }
    [[nodiscard]] QString path() const{return QDir::fromNativeSeparators(edit->text().trimmed());}

    template<class... PathSelectors, bool... directory>
    static void connect_signals(std::integer_sequence<bool, directory...>, PathSelectors *... selectors)
    {
        (QObject::connect(selectors->browse,&QPushButton::clicked,selectors,
        [selector = selectors, is_directory = directory]
            {
                const QString res = is_directory? QFileDialog::getExistingDirectory(selector, "Select Directory"): QFileDialog::getOpenFileName(selector, "Select File");
                if (!res.isEmpty())
                    selector->edit->setText(QDir::fromNativeSeparators(res));
            }),...);
    }

    QLineEdit *edit = nullptr;
    QPushButton *browse = nullptr;
};

class PluginBuilderPalette
    : public QWidget
{
    Q_OBJECT
public:
    explicit PluginBuilderPalette(QWidget *parent = nullptr);

private slots:
    /**
     * @brief 在系统文件管理器中打开当前输出目录。
     */
    void on_open_sandbox_dir();

private:
    void _setup_ui();
    void append_log(const QString &log){    m_log_edit->appendPlainText(log);}
    bool build();
    bool _execute_cmake(const QString &cmd,const QStringList &args,const QString &working_dir);
    void set_last_error(const QString &error);

    void _connect_signals();
    QHBoxLayout *m_layout;
    PathSelector *m_path_source;
    QPushButton   *m_build;
    QPushButton   *m_clean_build;
    QPushButton   *m_open_dir;
    QProgressBar *m_progress;
    QPlainTextEdit* m_log_edit;

    QString m_last_error;
    QString m_sandbox_dir;
};