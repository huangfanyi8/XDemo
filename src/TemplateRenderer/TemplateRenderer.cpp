#include "TemplateRenderer.h"

#include "../../mustache/mustache.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace DongDong
{

namespace _details
{

namespace fs = std::filesystem;

/**
 * @brief 读取 UTF-8 编码的文本文件
 */
std::optional<std::string> read_utf8_file(const fs::path &file_path, std::string &error_message)
{
    if (!fs::exists(file_path))
    {
        error_message = "模板文件不存在：" + file_path.string();
        return std::nullopt;
    }

    std::ifstream file(file_path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        error_message = "无法打开文件：" + file_path.string();
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * @brief 写入 UTF-8 编码的文本文件
 */
bool write_utf8_file(const fs::path &file_path, const std::string &content, std::string &error_message)
{
    // 确保父目录存在
    const auto parent_path = file_path.parent_path();
    if (!parent_path.empty() && !fs::exists(parent_path))
    {
        std::error_code ec;
        if (!fs::create_directories(parent_path, ec))
        {
            error_message = "无法创建目录：" + parent_path.string();
            return false;
        }
    }

    std::ofstream file(file_path, std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        error_message = "无法打开输出文件：" + file_path.string();
        return false;
    }

    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!file.good())
    {
        error_message = "写入文件失败：" + file_path.string();
        return false;
    }

    return true;
}

/**
 * @brief 将上下文数据转换为 Mustache 数据格式
 */
kainjow::mustache::data context_to_mustache_data(const TemplateRenderer::ContextMap &context)
{
    kainjow::mustache::data data(kainjow::mustache::data::type::object);

    for (const auto &[key, value] : context)
    {
        data.set(key, value);
    }

    return data;
}

} // namespace _details

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
    std::lock_guard<std::mutex> lock(m_mutex);
    m_error_message.clear();

    try
    {
        kainjow::mustache::mustache tmpl(template_text);
        const auto mustache_data = _details::context_to_mustache_data(context);
        return tmpl.render(mustache_data);
    }
    catch (const std::exception &e)
    {
        m_error_message = "模板渲染失败：" + std::string(e.what());
        return std::nullopt;
    }
    catch (...)
    {
        m_error_message = "模板渲染失败：未知异常";
        return std::nullopt;
    }
}

std::optional<std::string> TemplateRenderer::render_file_to_string(
    const std::string &template_file_path,
    const ContextMap &context)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_error_message.clear();

    auto template_text = _details::read_utf8_file(template_file_path, m_error_message);
    if (!template_text)
    {
        return std::nullopt;
    }

    // 释放锁，因为 render_to_string 会重新获取锁
    m_mutex.unlock();

    auto result = render_to_string(*template_text, context);

    m_mutex.lock();
    return result;
}

bool TemplateRenderer::render_file(
    const std::string &template_file_path,
    const std::string &output_file_path,
    const ContextMap &context)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_error_message.clear();

    auto template_text = _details::read_utf8_file(template_file_path, m_error_message);
    if (!template_text)
    {
        return false;
    }

    // 释放锁，因为后续操作需要重新获取锁
    m_mutex.unlock();

    auto rendered = render_to_string(*template_text, context);
    if (!rendered)
    {
        return false;
    }

    m_mutex.lock();
    return _details::write_utf8_file(output_file_path, *rendered, m_error_message);
}

} // namespace DongDong
