#include "TextEncodingHelper.h"

#include <QDir>
#include <QTextCodec>
#include <QVector>

namespace DongDong
{

QString TextEncodingHelper::decode_process_output(const QByteArray &output)
{
#ifdef Q_OS_WIN
    if (output.isEmpty())
    {
        return {};
    }

    QTextCodec::ConverterState state;
    QTextCodec *utf8_codec = QTextCodec::codecForName("UTF-8");
    if (utf8_codec != nullptr)
    {
        const QString utf8_text =
            utf8_codec->toUnicode(output.constData(), output.size(), &state);
        if (state.invalidChars == 0 && utf8_text.toUtf8() == output)
        {
            return utf8_text;
        }
    }

    return QString::fromLocal8Bit(output);
#else
    return QString::fromUtf8(output);
#endif
}

QString TextEncodingHelper::escape_cpp_u8_string_literal(const QString &value)
{
    QString escaped;
    const QVector<uint> code_points = value.toUcs4();
    escaped.reserve(static_cast<int>(code_points.size()));

    for (const uint code_point : code_points)
    {
        switch (code_point)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        default:
            if (code_point >= 0x20 && code_point <= 0x7E)
            {
                escaped += QChar(static_cast<ushort>(code_point));
            }
            else if (code_point <= 0xFFFF)
            {
                escaped += QString("\\u%1").arg(code_point, 4, 16, QChar('0')).toUpper();
            }
            else
            {
                escaped += QString("\\U%1").arg(code_point, 8, 16, QChar('0')).toUpper();
            }
            break;
        }
    }

    return escaped;
}

std::filesystem::path TextEncodingHelper::to_filesystem_path(const QString &path)
{
    const QByteArray utf8_path = QDir::fromNativeSeparators(path).toUtf8();
    return std::filesystem::u8path(utf8_path.constData(),
                                   utf8_path.constData() + utf8_path.size());
}

QString TextEncodingHelper::from_filesystem_path(const std::filesystem::path &path)
{
    const std::string utf8_path = path.u8string();
    return QDir::fromNativeSeparators(
        QString::fromUtf8(utf8_path.data(), static_cast<int>(utf8_path.size())));
}

std::string TextEncodingHelper::to_utf8_string(const QString &value)
{
    const QByteArray utf8_value = value.toUtf8();
    return std::string(utf8_value.constData(), static_cast<size_t>(utf8_value.size()));
}

std::string TextEncodingHelper::normalize_template_value(const QString &value)
{
    QString normalized = value;
    if (normalized.contains('\\'))
    {
        normalized = QDir::fromNativeSeparators(normalized);
    }

    return to_utf8_string(normalized);
}

bool TextEncodingHelper::should_write_utf8_bom(const std::filesystem::path &path)
{
    const std::string extension = path.extension().u8string();
    return extension == ".h"
           || extension == ".hpp"
           || extension == ".c"
           || extension == ".cc"
           || extension == ".cpp"
           || extension == ".cxx";
}

std::string TextEncodingHelper::path_to_utf8_string(const std::filesystem::path &path)
{
    return path.u8string();
}

} // namespace DongDong
