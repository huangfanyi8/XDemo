/**
 * @file TemplateRenderer.h
 * @brief 单文件 Mustache 模板渲染器，内置 Qt / std 路径与 UTF-8 适配
 */

#ifndef TEMPLATE_RENDERER_H
#define TEMPLATE_RENDERER_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <system_error>

#include <QString>
#include <QByteArray>

#include "../../mustache/mustache.hpp"

namespace DongDong
{
    namespace fs=std::filesystem;
    class  Renderer
    {
    public:
        static  bool renderer(const fs::path&template_input_path,
            const kainjow::mustache::data&context,
            const  fs::path &output_path)
        {
            //输入模板文件路径的校验，暂时不做

            auto ifs = std::ifstream(template_input_path,std::ios::binary);

            if (!ifs.is_open())
                return false;
        }

    private:
        std::string _error;
    };
}

#endif // TEMPLATE_RENDERER_H