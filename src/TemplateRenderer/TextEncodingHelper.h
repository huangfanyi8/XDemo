#ifndef TEXT_ENCODING_HELPER_H
#define TEXT_ENCODING_HELPER_H

#include <filesystem>
#include <string>

#include <QByteArray>
#include <QString>

namespace DongDong
{

class TextEncodingHelper
{
public:
    [[nodiscard]] static QString decode_process_output(const QByteArray &output);
    [[nodiscard]] static QString escape_cpp_u8_string_literal(const QString &value);
    [[nodiscard]] static std::filesystem::path to_filesystem_path(const QString &path);
    [[nodiscard]] static QString from_filesystem_path(const std::filesystem::path &path);
    [[nodiscard]] static std::string to_utf8_string(const QString &value);
    [[nodiscard]] static std::string normalize_template_value(const QString &value);
    [[nodiscard]] static bool should_write_utf8_bom(const std::filesystem::path &path);
    [[nodiscard]] static std::string path_to_utf8_string(const std::filesystem::path &path);
};

} // namespace DongDong

#endif
