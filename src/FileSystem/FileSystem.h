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