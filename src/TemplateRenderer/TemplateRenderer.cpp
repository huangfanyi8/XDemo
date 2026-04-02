#include "TemplateRenderer.h"

#include "TextEncodingHelper.h"

#include "../../mustache/mustache.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace DongDong
{

namespace fs = std::filesystem;

namespace
{

struct PluginTemplateFileMapping
{
    const char *template_name;
    fs::path output_name;
};

std::optional<std::string> read_utf8_file(const fs::path &file_path, std::string &error_message)
{
    std::error_code ec;
    if (!fs::exists(file_path, ec))
    {
        error_message = "模板文件不存在: " + TextEncodingHelper::path_to_utf8_string(file_path);
        return std::nullopt;
    }

    std::ifstream file(file_path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        error_message = "无法打开文件: " + TextEncodingHelper::path_to_utf8_string(file_path);
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool write_utf8_file(const fs::path &file_path,
                     const std::string &content,
                     const TemplateRenderer::WriteOptions &options,
                     std::string &error_message)
{
    const fs::path parent_path = file_path.parent_path();
    if (!parent_path.empty())
    {
        std::error_code ec;
        if (!fs::exists(parent_path, ec) && !fs::create_directories(parent_path, ec))
        {
            error_message = "无法创建目录: " + TextEncodingHelper::path_to_utf8_string(parent_path);
            return false;
        }
    }

    std::ofstream file(file_path, std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        error_message = "无法打开输出文件: " + TextEncodingHelper::path_to_utf8_string(file_path);
        return false;
    }

    if (options.write_utf8_bom)
    {
        static constexpr unsigned char utf8_bom[] = {0xEF, 0xBB, 0xBF};
        file.write(reinterpret_cast<const char *>(utf8_bom), sizeof(utf8_bom));
    }

    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!file.good())
    {
        error_message = "写入文件失败: " + TextEncodingHelper::path_to_utf8_string(file_path);
        return false;
    }

    return true;
}

kainjow::mustache::data context_to_mustache_data(const TemplateRenderer::ContextMap &context)
{
    kainjow::mustache::data data(kainjow::mustache::data::type::object);

    for (const auto &entry : context)
    {
        data.set(entry.first, entry.second);
    }

    return data;
}

void set_error_message(std::mutex &mutex,
                       std::string &error_message,
                       const std::string &value)
{
    std::lock_guard<std::mutex> lock(mutex);
    error_message = value;
}

TemplateRenderer::ContextMap make_plugin_template_context(
    const TemplateRenderer::PluginTemplateContext &context)
{
    return TemplateRenderer::ContextMap{
        {"CLASS_NAME", context.class_name},
        {"PLUGIN_NAME", context.plugin_name},
        {"VERSION", context.plugin_version},
        {"LABEL_TEXT", context.label_text}
    };
}

} // namespace

std::string TemplateRenderer::get_error() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_error_message;
}

void TemplateRenderer::clear_error()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_error_message.clear();
}

std::optional<std::string> TemplateRenderer::render_to_string(
    const std::string &template_text,
    const ContextMap &context)
{
    clear_error();

    try
    {
        kainjow::mustache::mustache tmpl(template_text);
        const auto mustache_data = context_to_mustache_data(context);
        return tmpl.render(mustache_data);
    }
    catch (const std::exception &e)
    {
        set_error_message(m_mutex, m_error_message, "模板渲染失败: " + std::string(e.what()));
        return std::nullopt;
    }
    catch (...)
    {
        set_error_message(m_mutex, m_error_message, "模板渲染失败: 未知异常");
        return std::nullopt;
    }
}

std::optional<std::string> TemplateRenderer::render_file_to_string(
    const std::string &template_file_path,
    const ContextMap &context)
{
    return render_file_to_string(fs::u8path(template_file_path), context);
}

std::optional<std::string> TemplateRenderer::render_file_to_string(
    const char *template_file_path,
    const ContextMap &context)
{
    return render_file_to_string(std::string(template_file_path), context);
}

std::optional<std::string> TemplateRenderer::render_file_to_string(
    const fs::path &template_file_path,
    const ContextMap &context)
{
    clear_error();

    std::string error_message;
    const auto template_text = read_utf8_file(template_file_path, error_message);
    if (!template_text)
    {
        set_error_message(m_mutex, m_error_message, error_message);
        return std::nullopt;
    }

    return render_to_string(*template_text, context);
}

bool TemplateRenderer::render_file(
    const std::string &template_file_path,
    const std::string &output_file_path,
    const ContextMap &context)
{
    return render_file(fs::u8path(template_file_path),
                       fs::u8path(output_file_path),
                       context,
                       {});
}

bool TemplateRenderer::render_file(
    const char *template_file_path,
    const char *output_file_path,
    const ContextMap &context)
{
    return render_file(std::string(template_file_path),
                       std::string(output_file_path),
                       context);
}

bool TemplateRenderer::render_file(
    const fs::path &template_file_path,
    const fs::path &output_file_path,
    const ContextMap &context,
    const WriteOptions &options)
{
    const auto rendered = render_file_to_string(template_file_path, context);
    if (!rendered)
    {
        return false;
    }

    std::string error_message;
    if (!write_utf8_file(output_file_path, *rendered, options, error_message))
    {
        set_error_message(m_mutex, m_error_message, error_message);
        return false;
    }

    clear_error();
    return true;
}

bool TemplateRenderer::generate_plugin_files(
    const fs::path &output_dir,
    const fs::path &template_dir,
    const fs::path &metadata_file_path,
    const PluginTemplateContext &context,
    std::vector<fs::path> *created_files)
{
    clear_error();

    const ContextMap template_context = make_plugin_template_context(context);
    const std::array<PluginTemplateFileMapping, 4> template_files{{
        {"IRuntimeComponentPlugin.h.in", "PluginInterfaceBase.h"},
        {"PluginClass.h.in", context.class_name + ".h"},
        {"PluginClass.cpp.in", context.class_name + ".cpp"},
        {"CMakeLists.txt.in", "CMakeLists.txt"}
    }};

    if (created_files != nullptr)
    {
        created_files->clear();
    }

    for (const auto &template_file : template_files)
    {
        const fs::path template_path = template_dir / template_file.template_name;
        const fs::path output_path = output_dir / template_file.output_name;
        WriteOptions options;
        options.write_utf8_bom = TextEncodingHelper::should_write_utf8_bom(output_path);

        if (!render_file(template_path, output_path, template_context, options))
        {
            return false;
        }

        if (created_files != nullptr)
        {
            created_files->push_back(output_path.filename());
        }
    }

    std::error_code ec;
    const fs::path copied_metadata_path = output_dir / "metadata.json";
    fs::copy_file(metadata_file_path,
                  copied_metadata_path,
                  fs::copy_options::overwrite_existing,
                  ec);
    if (ec)
    {
        set_error_message(m_mutex,
                          m_error_message,
                          "复制 metadata.json 失败: "
                          + TextEncodingHelper::path_to_utf8_string(copied_metadata_path));
        return false;
    }

    if (created_files != nullptr)
    {
        created_files->push_back(copied_metadata_path.filename());
    }

    clear_error();
    return true;
}

} // namespace DongDong
