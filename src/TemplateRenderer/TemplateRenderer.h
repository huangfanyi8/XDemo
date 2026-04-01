#ifndef TEMPLATE_RENDERER_H
#define TEMPLATE_RENDERER_H

#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>

namespace DongDong
{

class TemplateRenderer
{
public:
    using ContextMap = std::unordered_map<std::string, std::string>;

    TemplateRenderer() = default;

    /**
     * @brief 获取最后一次操作的错误信息
     * @return 错误信息字符串
     */
    std::string get_error() const;

    /**
     * @brief 清除错误信息
     */
    void clear_error();

    /**
     * @brief 将模板字符串渲染为结果字符串
     * @param template_text 模板文本内容
     * @param context 上下文数据（键值对）
     * @return 成功返回渲染结果，失败返回 std::nullopt
     */
    std::optional<std::string> render_to_string(
        const std::string &template_text,
        const ContextMap &context);

    /**
     * @brief 将模板文件渲染为字符串
     * @param template_file_path 模板文件路径（UTF-8 编码）
     * @param context 上下文数据
     * @return 成功返回渲染结果，失败返回 std::nullopt
     */
    std::optional<std::string> render_file_to_string(
        const std::string &template_file_path,
        const ContextMap &context);

    /**
     * @brief 将模板文件渲染并写入输出文件
     * @param template_file_path 模板文件路径（UTF-8 编码）
     * @param output_file_path 输出文件路径（UTF-8 编码）
     * @param context 上下文数据
     * @return 成功返回 true，失败返回 false
     */
    bool render_file(
        const std::string &template_file_path,
        const std::string &output_file_path,
        const ContextMap &context);

private:
    mutable std::string m_error_message;
    mutable std::mutex m_mutex;
};

} // namespace DongDong

#endif
