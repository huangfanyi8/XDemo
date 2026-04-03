#ifndef PLUGINBUILDER_H
#define PLUGINBUILDER_H

#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

struct PathSelector
    : public QWidget
{
    Q_OBJECT

public:
    explicit PathSelector(QWidget *parent = nullptr);

    [[nodiscard]] QString path() const
    {
        return QDir::fromNativeSeparators(edit->text().trimmed());
    }

    template<class... PathSelectors, bool... directory>
    static void _connect_signals(std::integer_sequence<bool, directory...>, PathSelectors *... selectors)
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

    QLineEdit *edit = nullptr;
    QPushButton *browse = nullptr;
};

class PluginBuilderView
    : public QWidget
{
    Q_OBJECT

public:
    explicit PluginBuilderView(QWidget *parent = nullptr);

private slots:
    void on_open_output_dir();

private:
    void _setup_ui();
    void _connect_signals();
    void append_log(const QString &log){    m_log_edit->appendPlainText(log);}
    bool build();
    bool _execute_cmake(const QString &cmd,const QStringList &args,const QString &working_dir);
    QString find_vcvars64() const;
    void set_last_error(const QString &error);

    template<class...String,std::enable_if_t<(std::is_same_v<std::decay_t<String>, QString> && ...),int> = 0>
    bool _validate_paths(const String&... paths)
    {
        return (... && [this](const QString& path)
        {
            if (QDir().mkpath(path))
                return true;
            set_last_error(path);
            return false;
        }(paths));
    }

    PathSelector *m_path_source = nullptr;
    QPushButton *m_build = nullptr;
    QPushButton *m_clean_build = nullptr;
    QProgressBar *m_progress = nullptr;
    QPlainTextEdit *m_log_edit = nullptr;

    QString m_last_error;
    QString m_sandbox_dir;//插件的构建沙箱环境
};

#endif // PLUGINBUILDER_H
