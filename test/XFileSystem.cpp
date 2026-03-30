#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <QString>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDataStream>
#include <QStandardPaths>
#include <QDateTime>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDirIterator>

class FileUtils
{
public:
    // ========== 路径获取 ==========

    // 程序所在目录
    static QString get_app_directory();

    // 用户桌面路径
    static QString get_desktop_directory();

    // 用户文档路径
    static QString get_documents_directory();

    // 应用数据目录（推荐存配置和用户数据）
    static QString get_app_data_directory(const QString &app_name = QString());

    // 临时文件目录
    static QString get_temp_directory();

    // 拼接路径（自动处理分隔符）
    static QString join_path(const QString &path1, const QString &path2);


    // ========== 目录操作 ==========

    // 确保目录存在（不存在则创建，支持多级）
    static bool ensure_directory_exists(const QString &path);

    // 删除空目录
    static bool remove_empty_directory(const QString &path);

    // 强制删除目录（包括内部所有内容）
    static bool remove_directory_recursive(const QString &path);


    // ========== 文件基础操作 ==========

    // 文件是否存在
    static bool file_exists(const QString &file_path);

    // 删除文件
    static bool remove_file(const QString &file_path);

    // 复制文件
    static bool copy_file(const QString &source_path, const QString &dest_path, bool overwrite = false);

    // 移动文件
    static bool move_file(const QString &source_path, const QString &dest_path, bool overwrite = false);

    // 重命名文件
    static bool rename_file(const QString &old_path, const QString &new_path);


    // ========== 文本文件读写 ==========

    // 读取整个文本文件
    static QString read_text_file(const QString &file_path, bool *ok = nullptr);

    // 写入文本文件（覆盖）
    static bool write_text_file(const QString &file_path, const QString &content);

    // 追加文本到文件（自动创建）
    static bool append_text_file(const QString &file_path, const QString &content);


    // ========== 二进制文件读写 ==========

    // 读取二进制文件
    static QByteArray read_binary_file(const QString &file_path, bool *ok = nullptr);

    // 写入二进制文件
    static bool write_binary_file(const QString &file_path, const QByteArray &data);


    // ========== 日志专用 ==========

    // 写入日志（自动加时间戳，自动创建目录）
    static bool write_log(const QString &log_dir, const QString &message);


    // ========== 批量操作 ==========

    // 获取目录下所有文件
    static QStringList get_files_in_directory(
        const QString &dir_path,
        const QStringList &name_filters = QStringList(),
        bool recursive = false
    );

    // 获取目录下所有子目录
    static QStringList get_subdirectories(const QString &dir_path, bool recursive = false);

    // 递归搜索指定后缀文件，并复制/移动到目标目录
    // operation: "copy" 复制, "move" 移动
    static bool collect_files_by_extension(
        const QString &source_dir,
        const QStringList &extensions,
        const QString &target_dir,
        const QString &operation = "copy",
        bool preserve_structure = false
    );

private:
    FileUtils();
};

#endif // FILE_UTILS_H

// ========== 路径获取 ==========

QString FileUtils::get_app_directory()
{
    return QCoreApplication::applicationDirPath();
}

QString FileUtils::get_desktop_directory()
{
    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

QString FileUtils::get_documents_directory()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString FileUtils::get_app_data_directory(const QString &app_name)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (!app_name.isEmpty())
    {
        path = join_path(path, app_name);
    }

    ensure_directory_exists(path);
    return path;
}

QString FileUtils::get_temp_directory()
{
    return QDir::tempPath();
}

QString FileUtils::join_path(const QString &path1, const QString &path2)
{
    return QDir(path1).filePath(path2);
}


// ========== 目录操作 ==========

bool FileUtils::ensure_directory_exists(const QString &path)
{
    if (path.isEmpty())
    {
        return false;
    }

    QDir dir;
    return dir.mkpath(path);
}

bool FileUtils::remove_empty_directory(const QString &path)
{
    QDir dir;
    return dir.rmdir(path);
}

bool FileUtils::remove_directory_recursive(const QString &path)
{
    QDir dir(path);
    return dir.removeRecursively();
}


// ========== 文件基础操作 ==========

bool FileUtils::file_exists(const QString &file_path)
{
    return QFile::exists(file_path);
}

bool FileUtils::remove_file(const QString &file_path)
{
    return QFile::remove(file_path);
}

bool FileUtils::copy_file(const QString &source_path, const QString &dest_path, bool overwrite)
{
    if (!file_exists(source_path))
    {
        return false;
    }

    if (file_exists(dest_path) && !overwrite)
    {
        return false;
    }

    QFileInfo dest_info(dest_path);
    ensure_directory_exists(dest_info.path());

    return QFile::copy(source_path, dest_path);
}

bool FileUtils::move_file(const QString &source_path, const QString &dest_path, bool overwrite)
{
    if (!file_exists(source_path))
    {
        return false;
    }

    if (file_exists(dest_path) && !overwrite)
    {
        return false;
    }

    QFileInfo dest_info(dest_path);
    ensure_directory_exists(dest_info.path());

    return QFile::rename(source_path, dest_path);
}

bool FileUtils::rename_file(const QString &old_path, const QString &new_path)
{
    return move_file(old_path, new_path, true);
}


// ========== 文本文件读写 ==========

QString FileUtils::read_text_file(const QString &file_path, bool *ok)
{
    QFile file(file_path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (ok)
        {
            *ok = false;
        }
        return QString();
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    if (ok)
    {
        *ok = true;
    }
    return content;
}

bool FileUtils::write_text_file(const QString &file_path, const QString &content)
{
    QFileInfo info(file_path);
    ensure_directory_exists(info.path());

    QFile file(file_path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        return false;
    }

    QTextStream out(&file);
    out << content;
    file.close();

    return true;
}

bool FileUtils::append_text_file(const QString &file_path, const QString &content)
{
    QFileInfo info(file_path);
    ensure_directory_exists(info.path());

    QFile file(file_path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
    {
        return false;
    }

    QTextStream out(&file);
    out << content;
    file.close();

    return true;
}


// ========== 二进制文件读写 ==========

QByteArray FileUtils::read_binary_file(const QString &file_path, bool *ok)
{
    QFile file(file_path);

    if (!file.open(QIODevice::ReadOnly))
    {
        if (ok)
        {
            *ok = false;
        }
        return QByteArray();
    }

    QByteArray data = file.readAll();
    file.close();

    if (ok)
    {
        *ok = true;
    }
    return data;
}

bool FileUtils::write_binary_file(const QString &file_path, const QByteArray &data)
{
    QFileInfo info(file_path);
    ensure_directory_exists(info.path());

    QFile file(file_path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }

    file.write(data);
    file.close();

    return true;
}


// ========== 日志专用 ==========

bool FileUtils::write_log(const QString &log_dir, const QString &message)
{
    ensure_directory_exists(log_dir);

    QString date_str = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString log_file = join_path(log_dir, date_str + ".log");

    QString time_str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString log_line = QString("[%1] %2\n").arg(time_str).arg(message);

    return append_text_file(log_file, log_line);
}


// ========== 批量操作 ==========

QStringList FileUtils::get_files_in_directory(
    const QString &dir_path,
    const QStringList &name_filters,
    bool recursive)
{
    QStringList result;
    QDir dir(dir_path);

    if (!dir.exists())
    {
        return result;
    }

    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot;

    if (recursive)
    {
        QDirIterator::IteratorFlags flags = QDirIterator::Subdirectories;
        QDirIterator it(dir_path, name_filters, filters, flags);

        while (it.hasNext())
        {
            result << it.next();
        }
    }
    else
    {
        if (name_filters.isEmpty())
        {
            for (const QFileInfo &info : dir.entryInfoList(filters))
            {
                result << info.absoluteFilePath();
            }
        }
        else
        {
            for (const QString &file : dir.entryList(name_filters, filters))
            {
                result << join_path(dir_path, file);
            }
        }
    }

    return result;
}

QStringList FileUtils::get_subdirectories(const QString &dir_path, bool recursive)
{
    QStringList result;
    QDir dir(dir_path);

    if (!dir.exists())
    {
        return result;
    }

    QDir::Filters filters = QDir::Dirs | QDir::NoDotAndDotDot;

    if (recursive)
    {
        QDirIterator it(dir_path, QStringList(), filters, QDirIterator::Subdirectories);

        while (it.hasNext())
        {
            result << it.next();
        }
    }
    else
    {
        for (const QFileInfo &info : dir.entryInfoList(filters))
        {
            result << info.absoluteFilePath();
        }
    }

    return result;
}

bool FileUtils::collect_files_by_extension(
    const QString &source_dir,
    const QStringList &extensions,
    const QString &target_dir,
    const QString &operation,
    bool preserve_structure)
{
    if (source_dir.isEmpty() || target_dir.isEmpty())
    {
        return false;
    }

    if (operation != "copy" && operation != "move")
    {
        return false;
    }

    QDir src_dir(source_dir);
    if (!src_dir.exists())
    {
        return false;
    }

    if (!ensure_directory_exists(target_dir))
    {
        return false;
    }

    QStringList files = get_files_in_directory(source_dir, extensions, true);

    if (files.isEmpty())
    {
        return true;
    }

    bool all_success = true;

    for (const QString &source_path : files)
    {
        QFileInfo source_info(source_path);
        QString file_name = source_info.fileName();

        QString dest_path;

        if (preserve_structure)
        {
            QString relative_path = src_dir.relativeFilePath(source_info.path());
            QString dest_dir = join_path(target_dir, relative_path);
            ensure_directory_exists(dest_dir);
            dest_path = join_path(dest_dir, file_name);
        }
        else
        {
            dest_path = join_path(target_dir, file_name);

            int counter = 1;
            QString base_name = source_info.completeBaseName();
            QString suffix = source_info.suffix();

            while (file_exists(dest_path))
            {
                QString new_name = QString("%1_%2.%3")
                    .arg(base_name)
                    .arg(counter)
                    .arg(suffix);
                dest_path = join_path(target_dir, new_name);
                counter++;
            }
        }

        bool success = false;

        if (operation == "copy")
        {
            success = copy_file(source_path, dest_path, false);
        }
        else
        {
            success = move_file(source_path, dest_path, false);
        }

        if (!success)
        {
            all_success = false;
        }
    }

    return all_success;
}