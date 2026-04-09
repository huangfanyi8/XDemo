#ifndef TEMPLATE_RENDERER_H
#define TEMPLATE_RENDERER_H

#include <string>
#include <vector>
#include <fstream>
#include <optional>
#include <filesystem>
#include <system_error>
#include "../../mustache/mustache.hpp"

#include <unordered_map>
#include <optional>
#include <mutex>




namespace DongDong
{

    namespace mustache =  kainjow::mustache;

    struct Diagnostic
    {
        void info(const std::string& message) { m_info_messages.push_back(message); }
        void warning(const std::string& message) { m_warning_messages.push_back(message); }
        void error(const std::string& message) { m_error_messages.push_back(message); }
        
        [[nodiscard]] const std::vector<std::string>& last_errors() const { return m_error_messages; }
        [[nodiscard]] bool has_errors() const { return !m_error_messages.empty(); }
        
    private:
        std::vector<std::string> m_info_messages;
        std::vector<std::string> m_warning_messages;
        std::vector<std::string> m_error_messages;
    };

    class TemplateRenderer
    {
    public:
        bool render_to(const std::filesystem::path& input_file_path,const std::filesystem::path& output_file_path,const mustache::data& context,Diagnostic*) const;
        bool render_to_directory(const std::filesystem::path& input_file_path,const std::filesystem::path& output_file_path,const mustache::data& context,Diagnostic*) const;
    private:
        bool _read(const std::filesystem::path& file_path,std::string& out_text,Diagnostic*) const;
        bool _render(const std::string& template_text,const mustache::data& context,std::string& out_text,Diagnostic*) const;
        bool _write(const std::filesystem::path& file_path,std::string_view content,Diagnostic*) const;
    };

}


#endif
