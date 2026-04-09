#ifndef PLUGINBUILDER_H
#define PLUGINBUILDER_H

#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
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
    void append_log(const QString &log){    m_log_edit->appendPlainText(QDir::toNativeSeparators(log));}
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



#endif // PLUGINBUILDER_H
