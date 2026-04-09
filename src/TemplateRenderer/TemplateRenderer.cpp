#include"TemplateRenderer.h"

#include "TemplateRenderer.h"

#include <fstream>
#include <sstream>

namespace DongDong
{

    bool TemplateRenderer::render_to(const std::filesystem::path& input_file_path,
                                     const std::filesystem::path& output_file_path,
                                     const mustache::data& context,
                                     Diagnostic* diagnostic) const
    {
        std::string template_text;
        if (!_read(input_file_path, template_text, diagnostic))
        {
            return false;
        }

        std::string rendered_text;
        if (!_render(template_text, context, rendered_text, diagnostic))
        {
            return false;
        }

        if (!_write(output_file_path, rendered_text, diagnostic))
        {
            return false;
        }

        return true;
    }

bool TemplateRenderer::render_to_directory(const std::filesystem::path& input_dir,
                                           const std::filesystem::path& output_dir,
                                           const mustache::data& context,
                                           Diagnostic* diagnostic) const
{
    // 输入必须是目录
    if (!std::filesystem::exists(input_dir) || !std::filesystem::is_directory(input_dir))
    {
        if (diagnostic) diagnostic->error("input directory does not exist or is not a directory: " + input_dir.string());
        return false;
    }

    // 输出若已存在，必须是目录
    if (std::filesystem::exists(output_dir) && !std::filesystem::is_directory(output_dir))
    {
        if (diagnostic) diagnostic->error("output path must be a directory: " + output_dir.string());
        return false;
    }

    try
    {
        std::filesystem::create_directories(output_dir);

        for (const auto& entry : std::filesystem::recursive_directory_iterator(input_dir))
        {
            if (!entry.is_regular_file()) continue;

            const std::filesystem::path in_file   = entry.path();
            const std::filesystem::path rel_path = std::filesystem::relative(in_file, input_dir);
            std::filesystem::path out_file = output_dir / rel_path;

            // ---- 关键修改点：循环去除所有 .in ----
            while (out_file.extension() == ".in")
                out_file.replace_extension();
            // -----------------------------------------

            // 确保父目录存在
            std::filesystem::create_directories(out_file.parent_path());

            if (!render_to(in_file, out_file, context, diagnostic))
            {
                if (diagnostic) diagnostic->error("failed to render file: " + in_file.string());
                return false;
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        if (diagnostic) diagnostic->error(std::string("failed to render directory: ") + e.what());
        return false;
    }
}


    bool TemplateRenderer::_read(const std::filesystem::path& file_path,
                                 std::string& out_text,
                                 Diagnostic* diagnostic) const
    {
        std::ifstream input(file_path, std::ios::binary);
        if (!input.is_open())
        {
            if (diagnostic)
            {
                diagnostic->error("failed to open template file: " + file_path.string());
            }
            return false;
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();

        if (input.bad())
        {
            if (diagnostic)
            {
                diagnostic->error("failed to read template file: " + file_path.string());
            }
            return false;
        }

        out_text = buffer.str();
        return true;
    }

    bool TemplateRenderer::_render(const std::string& template_text,
                                   const mustache::data& context,
                                   std::string& out_text,
                                   Diagnostic* diagnostic) const
    {
        try
        {
            mustache::mustache tmpl(template_text);
            out_text = tmpl.render(context);
            return true;
        }
        catch (const std::exception& e)
        {
            if (diagnostic)
            {
                diagnostic->error(std::string("failed to render template: ") + e.what());
            }
            return false;
        }
        catch (...)
        {
            if (diagnostic)
            {
                diagnostic->error("failed to render template: unknown error");
            }
            return false;
        }
    }

    bool TemplateRenderer::_write(const std::filesystem::path& file_path,
                                  std::string_view content,
                                  Diagnostic* diagnostic) const
    {
        std::ofstream output(file_path, std::ios::binary | std::ios::trunc);
        if (!output.is_open())
        {
            if (diagnostic)
            {
                diagnostic->error("failed to open output file: " + file_path.string());
            }
            return false;
        }

        output.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!output)
        {
            if (diagnostic)
            {
                diagnostic->error("failed to write output file: " + file_path.string());
            }
            return false;
        }

        return true;
    }

} // namespace DongDong