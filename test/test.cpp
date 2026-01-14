// node_to_code_test.cpp
// 编译: g++ -std=c++17 -o node_test node_to_code_test.cpp
// 运行: ./node_test

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <windows.h> // Windows编码转换需要

// ==================== 1. 核心数据结构定义 ====================
enum class node_type {
    seq_start,       // 序列开始
    var_decl,        // 变量声明 int x = 10
    var_ref,         // 变量引用 x
    number_literal,  // 数字字面量 10
    compare_op,      // 比较操作 >, <
    logic_op,        // 逻辑操作 &&, ||
    output           // 最终输出
};

struct graph_node {
    std::string id;
    node_type type;
    std::string value;
    std::string var_type; // 仅声明节点使用
    std::vector<std::string> input_ids; // 输入连接
    std::string output_id; // 输出连接

    graph_node(std::string i, node_type t, std::string v = "", std::string vt = "")
        : id(std::move(i)), type(t), value(std::move(v)), var_type(std::move(vt)) {}
};

// ==================== 2. 模拟节点图构建 ====================
// 这里用代码模拟你从UI获取的节点连接图
// 对应逻辑：int x=10; int y=2; (x>13) && (y<30)
std::unordered_map<std::string, std::unique_ptr<graph_node>> create_test_graph() {
    auto nodes = std::unordered_map<std::string, std::unique_ptr<graph_node>>();

    // 1. 声明节点
    nodes["decl_x"] = std::make_unique<graph_node>("decl_x", node_type::var_decl, "x", "int");
    nodes["decl_y"] = std::make_unique<graph_node>("decl_y", node_type::var_decl, "y", "int");

    // 2. 字面量节点（作为声明的初始值）
    nodes["lit_10"] = std::make_unique<graph_node>("lit_10", node_type::number_literal, "10");
    nodes["lit_2"] = std::make_unique<graph_node>("lit_2", node_type::number_literal, "2");
    nodes["lit_13"] = std::make_unique<graph_node>("lit_13", node_type::number_literal, "13");
    nodes["lit_30"] = std::make_unique<graph_node>("lit_30", node_type::number_literal, "30");

    // 3. 变量引用节点
    nodes["ref_x"] = std::make_unique<graph_node>("ref_x", node_type::var_ref, "x");
    nodes["ref_y"] = std::make_unique<graph_node>("ref_y", node_type::var_ref, "y");

    // 4. 比较操作节点
    nodes["cmp_gt"] = std::make_unique<graph_node>("cmp_gt", node_type::compare_op, ">");
    nodes["cmp_lt"] = std::make_unique<graph_node>("cmp_lt", node_type::compare_op, "<");

    // 5. 逻辑操作节点
    nodes["logic_and"] = std::make_unique<graph_node>("logic_and", node_type::logic_op, "&&");

    // 6. 输出节点
    nodes["output"] = std::make_unique<graph_node>("output", node_type::output, "");

    // ========== 建立连接关系 ==========
    // 声明节点的初始化连接
    nodes["decl_x"]->input_ids.push_back("lit_10");
    nodes["decl_y"]->input_ids.push_back("lit_2");

    // 比较操作连接：x > 13
    nodes["cmp_gt"]->input_ids.push_back("ref_x"); // 左操作数
    nodes["cmp_gt"]->input_ids.push_back("lit_13"); // 右操作数

    // 比较操作连接：y < 30
    nodes["cmp_lt"]->input_ids.push_back("ref_y"); // 左操作数
    nodes["cmp_lt"]->input_ids.push_back("lit_30"); // 右操作数

    // 逻辑操作连接：(x>13) && (y<30)
    nodes["logic_and"]->input_ids.push_back("cmp_gt"); // 左表达式
    nodes["logic_and"]->input_ids.push_back("cmp_lt"); // 右表达式

    // 输出连接
    nodes["output"]->input_ids.push_back("logic_and");

    return nodes;
}

// ==================== 3. AST节点定义 ====================
namespace ast {
class node {
public:
    virtual ~node() = default;
    virtual void generate_code(std::ostream& out, int indent = 0) const = 0;
};

class variable final : public node {
    std::string name;
public:
    explicit variable(std::string n) : name(std::move(n)) {}
    void generate_code(std::ostream& out, int /*indent*/) const override { out << name; }
};

class number final : public node {
    int value;
public:
    explicit number(int v) : value(v) {}
    void generate_code(std::ostream& out, int /*indent*/) const override { out << value; }
};

class binary_op final : public node {
    std::unique_ptr<node> left;
    std::unique_ptr<node> right;
    std::string op;
public:
    binary_op(std::unique_ptr<node> l, std::unique_ptr<node> r, std::string o)
        : left(std::move(l)), right(std::move(r)), op(std::move(o)) {}
    void generate_code(std::ostream& out, int /*indent*/) const override {
        out << "("; left->generate_code(out); out << " " << op << " "; right->generate_code(out); out << ")";
    }
};

class declaration final : public node {
    std::string type;
    std::string name;
    std::unique_ptr<node> init;
public:
    declaration(std::string t, std::string n, std::unique_ptr<node> i = nullptr)
        : type(std::move(t)), name(std::move(n)), init(std::move(i)) {}
    void generate_code(std::ostream& out, int /*indent*/) const override {
        out << type << " " << name;
        if (init) { out << " = "; init->generate_code(out); }
    }
};

class sequence final : public node {
    std::vector<std::unique_ptr<node>> statements;
    std::unique_ptr<node> final_expr;
public:
    void add_statement(std::unique_ptr<node> stmt) { statements.push_back(std::move(stmt)); }
    void set_final_expr(std::unique_ptr<node> expr) { final_expr = std::move(expr); }
    void generate_code(std::ostream& out, int indent = 0) const override {
        std::string indent_str(indent, ' ');
        for (const auto& stmt : statements) {
            out << indent_str; stmt->generate_code(out); out << ";\n";
        }
        if (final_expr) {
            out << indent_str << "return "; final_expr->generate_code(out); out << ";\n";
        }
    }
};
} // namespace ast

// ==================== 4. 图到AST的转换器 ====================
class graph_ast_builder {
    const std::unordered_map<std::string, std::unique_ptr<graph_node>>& graph;

    const graph_node* find_node(const std::string& id) const {
        auto it = graph.find(id);
        return (it != graph.end()) ? it->second.get() : nullptr;
    }

    std::unique_ptr<ast::node> build_from_id(const std::string& node_id) {
        const graph_node* node = find_node(node_id);
        if (!node) return nullptr;

        switch (node->type) {
            case node_type::var_ref:
                return std::make_unique<ast::variable>(node->value);
            case node_type::number_literal:
                return std::make_unique<ast::number>(std::stoi(node->value));
            case node_type::compare_op:
            case node_type::logic_op: {
                if (node->input_ids.size() < 2) return nullptr;
                auto left = build_from_id(node->input_ids[0]);
                auto right = build_from_id(node->input_ids[1]);
                if (!left || !right) return nullptr;
                return std::make_unique<ast::binary_op>(std::move(left), std::move(right), node->value);
            }
            case node_type::var_decl: {
                std::unique_ptr<ast::node> init = nullptr;
                if (!node->input_ids.empty()) {
                    init = build_from_id(node->input_ids[0]);
                }
                return std::make_unique<ast::declaration>(node->var_type, node->value, std::move(init));
            }
            case node_type::output:
                if (!node->input_ids.empty()) {
                    return build_from_id(node->input_ids[0]);
                }
                return nullptr;
            default:
                return nullptr;
        }
    }

public:
    explicit graph_ast_builder(const std::unordered_map<std::string, std::unique_ptr<graph_node>>& g) : graph(g) {}

    std::unique_ptr<ast::sequence> build() {
        auto seq = std::make_unique<ast::sequence>();

        // 1. 找出所有声明节点，按加入顺序处理
        std::vector<std::string> decl_order = {"decl_x", "decl_y"};
        for (const auto& decl_id : decl_order) {
            if (auto decl_node = find_node(decl_id)) {
                auto decl_ast = build_from_id(decl_id);
                if (decl_ast) {
                    seq->add_statement(std::move(decl_ast));
                }
            }
        }

        // 2. 找出输出节点，构建最终表达式
        if (auto output_node = find_node("output")) {
            auto final_expr = build_from_id("output");
            if (final_expr) {
                seq->set_final_expr(std::move(final_expr));
            }
        }

        return seq;
    }
};

// ==================== 5. 代码生成器 ====================
std::string generate_cpp_code(const ast::sequence& seq, const std::string& func_name) {
    std::ostringstream code;

    code << "#include <iostream>\n\n";
    code << "bool " << func_name << "()\n{\n";

    std::ostringstream body;
    seq.generate_code(body, 4);
    code << body.str();

    // 如果序列没有设置最终表达式，添加默认返回
    std::string body_str = body.str();
    if (body_str.find("return") == std::string::npos) {
        code << "    return false;\n";
    }

    code << "}\n\n";
    code << "int main()\n{\n";
    code << "    bool result = " << func_name << "();\n";
    code << "    std::cout << \"Result: \" << std::boolalpha << result << std::endl;\n";
    code << "    return 0;\n";
    code << "}\n";

    return code.str();
}

// ==================== 6. Windows编码转换 ====================
std::string utf8_to_gbk(const std::string& utf8_str) {
    if (utf8_str.empty()) return "";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    wchar_t* wstr = new wchar_t[wlen];
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, wstr, wlen);
    int len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    char* gbk_str = new char[len];
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, gbk_str, len, nullptr, nullptr);
    std::string result(gbk_str);
    delete[] wstr;
    delete[] gbk_str;
    return result;
}

// ==================== 7. 主程序 ====================
int main() {
    std::cout << utf8_to_gbk("=== 节点图到C++代码生成器 ===\n\n");

    // 1. 创建模拟的节点图
    std::cout << utf8_to_gbk("1. 创建节点图...\n");
    auto node_graph = create_test_graph();
    std::cout << utf8_to_gbk("   创建了 ") << node_graph.size() << utf8_to_gbk(" 个节点\n");

    // 2. 转换为AST
    std::cout << utf8_to_gbk("2. 转换节点图为AST...\n");
    graph_ast_builder builder(node_graph);
    auto ast_sequence = builder.build();
    if (!ast_sequence) {
        std::cerr << utf8_to_gbk("错误: AST构建失败\n");
        return 1;
    }
    std::cout << utf8_to_gbk("   AST构建成功\n");

    // 3. 生成C++代码
    std::cout << utf8_to_gbk("3. 生成C++代码...\n");
    std::string cpp_code = generate_cpp_code(*ast_sequence, "node_generated_function");

    // 4. 显示和保存
    std::cout << utf8_to_gbk("\n生成的代码:\n");
    std::cout << "========================================\n";
    std::cout << cpp_code;
    std::cout << "========================================\n";

    std::ofstream out_file("node_generated.cpp");
    if (out_file) {
        out_file << cpp_code;
        out_file.close();
        std::cout << utf8_to_gbk("\n代码已保存到: node_generated.cpp\n");

        std::cout << utf8_to_gbk("\n编译和运行:\n");
        std::cout << "  g++ -std=c++17 -o node_test_program node_generated.cpp\n";
        std::cout << "  ./node_test_program\n";
    }

    std::cout << utf8_to_gbk("\n测试完成。\n");
    return 0;
}