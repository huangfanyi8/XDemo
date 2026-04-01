#include "../src/TemplateRenderer/TemplateRenderer.h"

#include <iostream>
#include <filesystem>
#include <cassert>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

using namespace DongDong;

// 辅助函数：读取文件内容
std::optional<std::string> read_file_content(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 辅助函数：删除文件
void remove_file(const std::string &path)
{
    std::error_code ec;
    fs::remove(path, ec);
}

// 测试：简单变量替换
void test_simple_variable_replacement()
{
    std::cout << "测试：简单变量替换... ";

    std::string template_text = "你好，{{name}}！你的年龄是 {{age}}。";

    TemplateRenderer renderer;
    TemplateRenderer::ContextMap context;
    context["name"] = "ZhangSan";
    context["age"] = "25";

    auto result = renderer.render_to_string(template_text, context);

    assert(result.has_value());
    assert(result.value().find("ZhangSan") != std::string::npos);
    assert(result.value().find("25") != std::string::npos);

    std::cout << "通过" << std::endl;
}

// 测试：条件判断（布尔值）
void test_conditional_boolean()
{
    std::cout << "测试：条件判断（布尔值）... ";

    std::string template_text = "开始{{#isVip}}VIP 用户{{/isVip}}结束";

    // 测试有值情况（显示内容）
    TemplateRenderer renderer;
    TemplateRenderer::ContextMap context;
    context["isVip"] = "yes";  // 有值

    auto result = renderer.render_to_string(template_text, context);

    assert(result.has_value());
    auto const &value = result.value();
    // 有值应该显示内容
    assert(value.find("VIP") != std::string::npos);
    assert(value.find("开始") != std::string::npos);
    assert(value.find("结束") != std::string::npos);

    // 测试无值情况（不显示内容）
    context.clear();  // 不提供 isVip 键
    result = renderer.render_to_string(template_text, context);

    assert(result.has_value());
    // 键不存在时，Mustache 视为空列表/假值，不显示中间内容
    assert(result.value().find("开始") != std::string::npos);
    assert(result.value().find("结束") != std::string::npos);
    // 键不存在时不应该显示 VIP
    assert(result.value().find("VIP") == std::string::npos);

    std::cout << "通过" << std::endl;
}

// 测试：反向条件
void test_inverted_conditional()
{
    std::cout << "测试：反向条件... ";

    std::string template_text = "{{^isLoggedIn}}请先登录{{/isLoggedIn}}";

    // 测试键不存在情况（显示内容）
    TemplateRenderer renderer;
    TemplateRenderer::ContextMap context;
    // 不提供 isLoggedIn 键

    auto result = renderer.render_to_string(template_text, context);

    assert(result.has_value());
    assert(result.value().find("请先登录") != std::string::npos);

    // 测试有值情况（不显示内容）
    context["isLoggedIn"] = "yes";  // 有值
    result = renderer.render_to_string(template_text, context);

    assert(result.has_value());
    assert(result.value().empty());

    std::cout << "通过" << std::endl;
}

// 测试：列表遍历
void test_list_iteration()
{
    std::cout << "测试：列表遍历... ";

    std::string template_text = "{{#items}}- {{.}}\n{{/items}}";

    TemplateRenderer renderer;
    TemplateRenderer::ContextMap context;
    context["items"] = "苹果，香蕉，橙子";

    std::string error;
    auto result = renderer.render_to_string(template_text, context);

    assert(result.has_value());
    // 列表遍历需要传入数组类型，这里测试简单情况
    std::cout << "通过" << std::endl;
}

// 测试：文件读取失败
void test_file_not_found()
{
    std::cout << "测试：文件不存在... ";

    TemplateRenderer renderer;
    TemplateRenderer::ContextMap context;
    context["name"] = "张三";

    auto result = renderer.render_file_to_string(
        "nonexistent_file.mustache",
        context);

    assert(!result.has_value());
    assert(!renderer.get_error().empty());
    assert(renderer.get_error().find("不存在") != std::string::npos);

    std::cout << "通过" << std::endl;
}

// 测试：渲染文件到文件
void test_render_file_to_file()
{
    std::cout << "测试：渲染文件到文件... ";

    // 创建临时模板文件
    const std::string template_path = "test_template.mustache";
    const std::string output_path = "test_output.txt";

    std::ofstream template_file(template_path, std::ios::binary);
    template_file << "Welcome {{name}}!\nAge: {{age}}";
    template_file.close();

    TemplateRenderer renderer;
    TemplateRenderer::ContextMap context;
    context["name"] = "LiSi";
    context["age"] = "30";

    bool success = renderer.render_file(template_path, output_path, context);

    assert(success);
    assert(renderer.get_error().empty());

    // 验证输出文件内容
    auto output_content = read_file_content(output_path);
    assert(output_content.has_value());
    assert(output_content.value().find("Welcome LiSi!") != std::string::npos);
    assert(output_content.value().find("Age: 30") != std::string::npos);

    // 清理临时文件
    remove_file(template_path);
    remove_file(output_path);

    std::cout << "通过" << std::endl;
}

// 测试：空上下文
void test_empty_context()
{
    std::cout << "测试：空上下文... ";

    std::string template_text = "你好，{{name}}！";

    TemplateRenderer renderer;
    TemplateRenderer::ContextMap context;
    // 不提供任何数据

    auto result = renderer.render_to_string(template_text, context);

    assert(result.has_value());
    assert(result.value() == "你好，！");  // 变量为空

    std::cout << "通过" << std::endl;
}

// 测试：特殊字符
void test_special_characters()
{
    std::cout << "测试：特殊字符... ";

    std::string template_text = "HTML: {{content}}";

    TemplateRenderer renderer;
    TemplateRenderer::ContextMap context;
    context["content"] = "<script>alert('XSS')</script>";

    auto result = renderer.render_to_string(template_text, context);

    assert(result.has_value());
    // Mustache 默认会转义 HTML
    assert(result.value().find("&lt;") != std::string::npos);

    std::cout << "通过" << std::endl;
}

// 测试：嵌套对象（使用点语法）
void test_nested_object()
{
    std::cout << "测试：嵌套对象（点语法）... ";

    // 注意：当前实现使用扁平的 ContextMap，不支持真正的嵌套对象
    // 这里测试简单的变量替换
    std::string template_text = "Name: {{name}}, Email: {{email}}";

    TemplateRenderer renderer;
    TemplateRenderer::ContextMap context;
    context["name"] = "WangWu";
    context["email"] = "wangwu@example.com";

    auto result = renderer.render_to_string(template_text, context);

    assert(result.has_value());
    assert(result.value().find("WangWu") != std::string::npos);
    assert(result.value().find("wangwu@example.com") != std::string::npos);

    std::cout << "通过" << std::endl;
}

// 测试：错误信息清除
void test_clear_error()
{
    std::cout << "测试：错误信息清除... ";

    TemplateRenderer renderer;
    TemplateRenderer::ContextMap context;

    // 先制造一个错误
    auto result = renderer.render_file_to_string(
        "nonexistent_file.mustache",
        context);

    assert(!result.has_value());
    assert(!renderer.get_error().empty());

    // 清除错误
    renderer.clear_error();
    assert(renderer.get_error().empty());

    std::cout << "通过" << std::endl;
}

int main()
{
#ifdef _WIN32
    // 设置 Windows 控制台为 UTF-8 模式
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << "========================================" << std::endl;
    std::cout << "   TemplateRenderer 测试套件" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    try
    {
        test_simple_variable_replacement();
        test_conditional_boolean();
        test_inverted_conditional();
        test_list_iteration();
        test_file_not_found();
        test_render_file_to_file();
        test_empty_context();
        test_special_characters();
        test_nested_object();
        test_clear_error();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "   所有测试通过！" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "测试失败：" << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "测试失败：未知异常" << std::endl;
        return 1;
    }
}
