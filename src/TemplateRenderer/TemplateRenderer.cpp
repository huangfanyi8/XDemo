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

    bool TemplateRenderer::render_to_directory(const std::filesystem::path& input_file_path,
                                               const std::filesystem::path& output_file_path,
                                               const mustache::data& context,
                                               Diagnostic* diagnostic) const
    {
        const std::filesystem::path& input_dir = input_file_path;
        const std::filesystem::path& output_dir = output_file_path;

        try
        {
            if (!std::filesystem::exists(input_dir))
            {
                if (diagnostic)
                {
                    diagnostic->error("input directory does not exist: " + input_dir.string());
                }
                return false;
            }

            if (!std::filesystem::is_directory(input_dir))
            {
                if (diagnostic)
                {
                    diagnostic->error("input path is not a directory: " + input_dir.string());
                }
                return false;
            }

            std::filesystem::create_directories(output_dir);

            for (const auto& entry : std::filesystem::recursive_directory_iterator(input_dir))
            {
                if (!entry.is_regular_file())
                {
                    continue;
                }

                const std::filesystem::path current_input_file = entry.path();
                const std::filesystem::path relative_path = std::filesystem::relative(current_input_file, input_dir);

                std::filesystem::path current_output_file = output_dir / relative_path;

                if (current_output_file.extension() == ".mustache")
                {
                    current_output_file.replace_extension();
                }

                const std::filesystem::path parent_dir = current_output_file.parent_path();
                if (!parent_dir.empty())
                {
                    std::filesystem::create_directories(parent_dir);
                }

                if (!render_to(current_input_file, current_output_file, context, diagnostic))
                {
                    if (diagnostic)
                    {
                        diagnostic->error("failed to render file: " + current_input_file.string());
                    }
                    return false;
                }
            }

            return true;
        }
        catch (const std::exception& e)
        {
            if (diagnostic)
            {
                diagnostic->error(std::string("failed to render directory: ") + e.what());
            }
            return false;
        }
        catch (...)
        {
            if (diagnostic)
            {
                diagnostic->error("failed to render directory: unknown error");
            }
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