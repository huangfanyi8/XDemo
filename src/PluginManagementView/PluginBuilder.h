#ifndef PLUGINBUILDER_H
#define PLUGINBUILDER_H

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
    void on_open_output_dir();

private:
    void _setup_ui();
    void append_log(const QString &log){    m_log_edit->appendPlainText(log);}
    bool build();
    bool _execute_cmake(const QString &cmd,const QStringList &args,const QString &working_dir);
    void set_last_error(const QString &error);




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
    QPushButton   *m_build;
    QPushButton   *m_clean_build;
    QPushButton   *m_open_dir;
    QProgressBar *m_progress;
    QPlainTextEdit* m_log_edit;

    QString m_last_error;
    QString m_sandbox_dir;
};

class PluginBuilder
{

};

#endif // PLUGINBUILDER_H
