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
#include <QJsonDocument>
#include <QJsonObject>

class PathSelector : public QWidget
{
    Q_OBJECT
public:
    enum Mode { Directory, File };
    explicit PathSelector(Mode m, QWidget *parent = nullptr);
    QString path() const{return m_edit->text().trimmed();}
private:
    QLineEdit *m_edit;
    Mode       m_mode;
};

class PluginBuilderView : public QWidget
{
    Q_OBJECT
public:
    explicit PluginBuilderView(QWidget *parent = nullptr);

private slots:
    /**
     * @brief Open the generated plugin output directory in the system file explorer.
     */
    void on_open_output_dir();

private:
    void setup_ui();

    /**
     * @brief Generate plugin wrapper files and build the plugin with CMake.
     *
     * The function validates the user-selected source, CMake, and Qt paths,
     * reads the plugin metadata, generates the temporary plugin project files,
     * configures the generated project with CMake, and builds the plugin.
     *
     * @param clean_first Set to @c true to remove the previous generated build
     *        directory before configuring again.
     * @return @c true if file generation, CMake configure, and CMake build all
     *         succeed; otherwise @c false.
     */
    bool execute_build(bool clean_first = false);

    /**
     * @brief Run an external command and append its output to the log view.
     *
     * @param cmd Executable path used to start the process.
     * @param args Command line arguments passed to the executable.
     * @param working_dir Working directory used when launching the process.
     * @return @c true if the process finishes successfully; otherwise @c false.
     */
    bool run_process(const QString &cmd, const QStringList &args, const QString &working_dir);

    /**
     * @brief Resolve the Visual Studio environment initialization script used for MSVC builds.
     *
     * @return Absolute path to @c vcvars64.bat when it can be found; otherwise
     *         an empty string.
     */
    QString find_vcvars64() const;

    /**
     * @brief Run a command after loading the MSVC and Windows SDK environment.
     *
     * This helper wraps the target command with a call to @c vcvars64.bat so
     * the process can find system libraries such as @c kernel32.lib.
     *
     * @param cmd Executable path used to start the process.
     * @param args Command line arguments passed to the executable.
     * @param working_dir Working directory used when launching the process.
     * @return @c true if the wrapped process finishes successfully; otherwise
     *         @c false.
     */
    bool run_process_with_msvc_environment(const QString &cmd,
                                           const QStringList &args,
                                           const QString &working_dir);
    void write_file(const QString &path, const QString &content);
    void append_log(const QString &log);

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
