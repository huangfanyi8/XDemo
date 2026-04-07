/**
 * @file TemplateRenderer.h
 * @brief 单文件 Mustache 模板渲染器，内置 Qt / std 路径与 UTF-8 适配
 */

#ifndef TEMPLATE_RENDERER_H
#define TEMPLATE_RENDERER_H

#include <string>
#include <vector>
#include <fstream>
#include <optional>
#include <filesystem>
#include <system_error>

#include <QString>
#include <QByteArray>

#include "../../mustache/mustache.hpp"



namespace DongDong
{
    namespace mustache =  kainjow::mustache;

    struct Diagnostic
    {
        void info(const std::string& message){}
        void warning(const std::string& message){}
        void error(const std::string& message){}
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

#endif // TEMPLATE_RENDERER_H