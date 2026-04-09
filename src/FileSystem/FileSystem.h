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
#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QFileInfoList>
#include <QtGlobal>

namespace DongDong
{
    /**
     * @brief 路径与文件系统工具类。
     *
     * 当前版本只处理普通路径：
     * - 内部统一使用 '/'
     * - 特殊路径预留接口，暂不处理
     */
    class PathManager
    {
    public:
        /**
         * @brief 错误类型。
         */
        enum class Error
        {
            None,
            EmptyPath,
            SourceNotFound,
            SourceIsNotFile,
            SourceIsNotDirectory,
            TargetAlreadyExists,
            TargetIsFile,
            TargetIsDirectory,
            CreateDirectoryFailed,
            CopyFailed,
            MoveFailed,
            RemoveFailed,
            UnsupportedSpecialPath
        };

    public:
        PathManager()
            : m_error(Error::None)
        {
        }

        /**
         * @brief 清空错误状态。
         */
        void clear_error()
        {
            m_error = Error::None;
            m_error_path.clear();
            m_error_detail.clear();
        }

        /**
         * @brief 获取当前错误类型。
         */
        Error error() const
        {
            return m_error;
        }

        /**
         * @brief 获取出错路径。
         */
        QString error_path() const
        {
            return m_error_path;
        }

        /**
         * @brief 获取错误说明。
         */
        QString error_string() const
        {
            switch (m_error)
            {
                case Error::None:
                    return QStringLiteral("No error");
                case Error::EmptyPath:
                    return QStringLiteral("Path is empty");
                case Error::SourceNotFound:
                    return QStringLiteral("Source path does not exist");
                case Error::SourceIsNotFile:
                    return QStringLiteral("Source path is not a file");
                case Error::SourceIsNotDirectory:
                    return QStringLiteral("Source path is not a directory");
                case Error::TargetAlreadyExists:
                    return QStringLiteral("Target path already exists");
                case Error::TargetIsFile:
                    return QStringLiteral("Target path is a file");
                case Error::TargetIsDirectory:
                    return QStringLiteral("Target path is a directory");
                case Error::CreateDirectoryFailed:
                    return QStringLiteral("Failed to create directory");
                case Error::CopyFailed:
                    return QStringLiteral("Failed to copy");
                case Error::MoveFailed:
                    return QStringLiteral("Failed to move");
                case Error::RemoveFailed:
                    return QStringLiteral("Failed to remove");
                case Error::UnsupportedSpecialPath:
                    return QStringLiteral("Unsupported special path");
            }

            return QStringLiteral("Unknown error");
        }

        /**
         * @brief 规范化路径，内部统一为 '/'。
         */
        QString normalize_path(const QString& raw_path) const
        {
            if (raw_path.trimmed().isEmpty())
            {
                return {};
            }

            if (_is_special_path(raw_path))
            {
                return _normalize_special_path(raw_path);
            }

            return _normalize_normal_path(raw_path);
        }

        /**
         * @brief 转为系统原生分隔符。
         */
        QString to_native_path(const QString& path) const
        {
            return QDir::toNativeSeparators(normalize_path(path));
        }

        /**
         * @brief 拼接路径。
         */
        QString join_path(const QString& base_path, const QString& child_path) const
        {
            const QString base = normalize_path(base_path);
            const QString child = normalize_path(child_path);

            if (base.isEmpty())
            {
                return child;
            }

            if (child.isEmpty())
            {
                return base;
            }

            return normalize_path(QDir(base).filePath(child));
        }

        /**
         * @brief 获取文件名。
         */
        QString file_name(const QString& path) const
        {
            return QFileInfo(normalize_path(path)).fileName();
        }

        /**
         * @brief 获取不带后缀的文件名。
         */
        QString base_name(const QString& path) const
        {
            return QFileInfo(normalize_path(path)).completeBaseName();
        }

        /**
         * @brief 获取后缀名。
         */
        QString suffix(const QString& path) const
        {
            return QFileInfo(normalize_path(path)).suffix();
        }

        /**
         * @brief 获取父目录。
         */
        QString parent_path(const QString& path) const
        {
            const QString normalized_path = normalize_path(path);

            if (normalized_path.isEmpty())
            {
                return {};
            }

            return normalize_path(QFileInfo(normalized_path).path());
        }

        /**
         * @brief 获取绝对路径。
         */
        QString absolute_path(const QString& path) const
        {
            const QString normalized_path = normalize_path(path);

            if (normalized_path.isEmpty())
            {
                return {};
            }

            return normalize_path(QFileInfo(normalized_path).absoluteFilePath());
        }

        /**
         * @brief 判断路径是否存在。
         */
        bool exists(const QString& path) const
        {
            const QString normalized_path = normalize_path(path);
            return !normalized_path.isEmpty() && QFileInfo::exists(normalized_path);
        }

        /**
         * @brief 判断是否为文件。
         */
        bool is_file(const QString& path) const
        {
            const QFileInfo info(normalize_path(path));
            return info.exists() && info.isFile();
        }

        /**
         * @brief 判断是否为目录。
         */
        bool is_directory(const QString& path) const
        {
            const QFileInfo info(normalize_path(path));
            return info.exists() && info.isDir();
        }

        /**
         * @brief 比较两个路径是否相等。
         */
        bool path_equals(const QString& left_path, const QString& right_path) const
        {
            const QString left = normalize_path(left_path);
            const QString right = normalize_path(right_path);

#ifdef Q_OS_WIN
            return QString::compare(left, right, Qt::CaseInsensitive) == 0;
#else
            return left == right;
#endif
        }

        /**
         * @brief 创建目录。
         */
        bool create_directories(const QString& directory_path)
        {
            clear_error();

            const QString path = normalize_path(directory_path);

            if (path.isEmpty())
            {
                return _set_error(Error::EmptyPath, directory_path);
            }

            if (_is_special_path(path))
            {
                return _set_error(Error::UnsupportedSpecialPath, path);
            }

            return QDir(path).exists() || QDir().mkpath(path) || _set_error(Error::CreateDirectoryFailed, path);
        }

        /**
         * @brief 确保父目录存在。
         */
        bool ensure_parent_directory(const QString& path)
        {
            clear_error();
            return _ensure_parent_directory_of_path(path);
        }

        /**
         * @brief 删除文件。
         */
        bool remove_file(const QString& file_path)
        {
            clear_error();

            const QString path = normalize_path(file_path);
            const QFileInfo info(path);

            if (path.isEmpty())
            {
                return _set_error(Error::EmptyPath, file_path);
            }

            if (!info.exists())
            {
                return true;
            }

            if (!info.isFile())
            {
                return _set_error(Error::SourceIsNotFile, path);
            }

            return QFile::remove(path) || _set_error(Error::RemoveFailed, path);
        }

        /**
         * @brief 删除目录。
         * @param recursive 是否递归删除。
         */
        bool remove_directory(const QString& directory_path, bool recursive = true)
        {
            clear_error();

            const QString path = normalize_path(directory_path);
            const QFileInfo info(path);

            if (path.isEmpty())
            {
                return _set_error(Error::EmptyPath, directory_path);
            }

            if (!info.exists())
            {
                return true;
            }

            if (!info.isDir())
            {
                return _set_error(Error::SourceIsNotDirectory, path);
            }

            if (recursive)
            {
                return QDir(path).removeRecursively() || _set_error(Error::RemoveFailed, path);
            }

            return QDir().rmdir(path) || _set_error(Error::RemoveFailed, path);
        }

        /**
         * @brief 拷贝文件。
         * @param overwrite 目标存在时是否覆盖。
         */
        bool copy_file(const QString& source_file_path,
                       const QString& target_file_path,
                       bool overwrite = false)
        {
            clear_error();

            const QString source_path = normalize_path(source_file_path);
            const QString target_path = normalize_path(target_file_path);
            const QFileInfo source_info(source_path);

            if (source_path.isEmpty() || target_path.isEmpty())
            {
                return _set_error(Error::EmptyPath, source_path.isEmpty() ? source_file_path : target_file_path);
            }

            if (path_equals(source_path, target_path))
            {
                return true;
            }

            if (!source_info.exists())
            {
                return _set_error(Error::SourceNotFound, source_path);
            }

            if (!source_info.isFile())
            {
                return _set_error(Error::SourceIsNotFile, source_path);
            }

            if (!_ensure_parent_directory_of_path(target_path))
            {
                return false;
            }

            if (!_prepare_target_file(target_path, overwrite))
            {
                return false;
            }

            return QFile::copy(source_path, target_path) || _set_error(Error::CopyFailed, source_path);
        }

        /**
         * @brief 移动文件。
         * @param overwrite 目标存在时是否覆盖。
         */
        bool move_file(const QString& source_file_path,
                       const QString& target_file_path,
                       bool overwrite = false)
        {
            clear_error();

            const QString source_path = normalize_path(source_file_path);
            const QString target_path = normalize_path(target_file_path);
            const QFileInfo source_info(source_path);

            if (source_path.isEmpty() || target_path.isEmpty())
            {
                return _set_error(Error::EmptyPath, source_path.isEmpty() ? source_file_path : target_file_path);
            }

            if (path_equals(source_path, target_path))
            {
                return true;
            }

            if (!source_info.exists())
            {
                return _set_error(Error::SourceNotFound, source_path);
            }

            if (!source_info.isFile())
            {
                return _set_error(Error::SourceIsNotFile, source_path);
            }

            if (!_ensure_parent_directory_of_path(target_path))
            {
                return false;
            }

            if (!_prepare_target_file(target_path, overwrite))
            {
                return false;
            }

            if (QFile::rename(source_path, target_path))
            {
                return true;
            }

            if (!QFile::copy(source_path, target_path))
            {
                return _set_error(Error::MoveFailed, source_path);
            }

            if (!QFile::remove(source_path))
            {
                return _set_error(Error::MoveFailed, source_path);
            }

            return true;
        }

        /**
         * @brief 拷贝目录。
         * @param overwrite 目标存在时是否覆盖。
         */
        bool copy_directory(const QString& source_directory_path,
                            const QString& target_directory_path,
                            bool overwrite = false)
        {
            clear_error();

            const QString source_path = normalize_path(source_directory_path);
            const QString target_path = normalize_path(target_directory_path);
            const QFileInfo source_info(source_path);

            if (source_path.isEmpty() || target_path.isEmpty())
            {
                return _set_error(Error::EmptyPath, source_path.isEmpty() ? source_directory_path : target_directory_path);
            }

            if (path_equals(source_path, target_path))
            {
                return true;
            }

            if (!source_info.exists())
            {
                return _set_error(Error::SourceNotFound, source_path);
            }

            if (!source_info.isDir())
            {
                return _set_error(Error::SourceIsNotDirectory, source_path);
            }

            if (!_prepare_target_directory(target_path, overwrite))
            {
                return false;
            }

            if (!create_directories(target_path))
            {
                return false;
            }

            const QFileInfoList entries = QDir(source_path).entryInfoList(
                QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System);

            for (const QFileInfo& entry : entries)
            {
                const QString source_entry_path = normalize_path(entry.absoluteFilePath());
                const QString target_entry_path = join_path(target_path, entry.fileName());

                if (entry.isDir())
                {
                    if (!copy_directory(source_entry_path, target_entry_path, false))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!copy_file(source_entry_path, target_entry_path, true))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        /**
         * @brief 移动目录。
         * @param source_directory_path
         * @param target_directory_path
         * @param overwrite 目标存在时是否覆盖。
         */
        bool move_directory(const QString& source_directory_path,
                            const QString& target_directory_path,
                            bool overwrite = false)
        {
            clear_error();

            const QString source_path = normalize_path(source_directory_path);
            const QString target_path = normalize_path(target_directory_path);
            const QFileInfo source_info(source_path);

            if (source_path.isEmpty() || target_path.isEmpty())
            {
                return _set_error(Error::EmptyPath, source_path.isEmpty() ? source_directory_path : target_directory_path);
            }

            if (path_equals(source_path, target_path))
            {
                return true;
            }

            if (!source_info.exists())
            {
                return _set_error(Error::SourceNotFound, source_path);
            }

            if (!source_info.isDir())
            {
                return _set_error(Error::SourceIsNotDirectory, source_path);
            }

            if (!_prepare_target_directory(target_path, overwrite))
            {
                return false;
            }

            if (!_ensure_parent_directory_of_path(target_path))
            {
                return false;
            }

            if (QDir().rename(source_path, target_path))
            {
                return true;
            }

            if (!copy_directory(source_path, target_path, false))
            {
                return _set_error(Error::MoveFailed, source_path);
            }

            if (!remove_directory(source_path, true))
            {
                return _set_error(Error::MoveFailed, source_path);
            }

            return true;
        }

    private:
        QString _normalize_normal_path(const QString& raw_path) const
        {
            return QDir::cleanPath(QDir::fromNativeSeparators(raw_path.trimmed()));
        }

        bool _is_special_path(const QString& raw_path) const
        {
            Q_UNUSED(raw_path);
            return false;
        }

        QString _normalize_special_path(const QString& raw_path) const
        {
            return raw_path;
        }

        bool _ensure_parent_directory_of_path(const QString& path)
        {
            const QString parent = parent_path(path);

            if (parent.isEmpty() || parent == QStringLiteral("."))
            {
                return true;
            }

            if (_is_special_path(parent))
            {
                return _set_error(Error::UnsupportedSpecialPath, parent);
            }

            return QDir(parent).exists() || QDir().mkpath(parent) || _set_error(Error::CreateDirectoryFailed, parent);
        }

        bool _prepare_target_file(const QString& target_file_path, bool overwrite)
        {
            const QFileInfo info(target_file_path);

            if (!info.exists())
            {
                return true;
            }

            if (info.isDir())
            {
                return _set_error(Error::TargetIsDirectory, target_file_path);
            }

            if (!overwrite)
            {
                return _set_error(Error::TargetAlreadyExists, target_file_path);
            }

            return QFile::remove(target_file_path) || _set_error(Error::RemoveFailed, target_file_path);
        }

        bool _prepare_target_directory(const QString& target_directory_path, bool overwrite)
        {
            const QFileInfo info(target_directory_path);

            if (!info.exists())
            {
                return true;
            }

            if (!info.isDir())
            {
                return _set_error(Error::TargetIsFile, target_directory_path);
            }

            if (!overwrite)
            {
                return _set_error(Error::TargetAlreadyExists, target_directory_path);
            }

            return QDir(target_directory_path).removeRecursively() || _set_error(Error::RemoveFailed, target_directory_path);
        }

        bool _set_error(Error error_type, const QString& path, const QString& detail = QString())
        {
            m_error = error_type;
            m_error_path = path;
            m_error_detail = detail;
            return false;
        }

    private:
        Error m_error;
        QString m_error_path;
        QString m_error_detail;
    };
}
#endif // FILE_UTILS_H