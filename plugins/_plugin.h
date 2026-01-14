#ifndef NODE_PLUGIN_H
#define NODE_PLUGIN_H

#include <QObject>
#include "common/_interface.h"
#include"../include/NodeEditor.h"


/**
 * @brief 节点编辑器插件入口类
 * 必须继承 QObject 并实现 node_editor_interface 接口
 */
class node_plugin : public QObject, public node_editor_interface
{
    Q_OBJECT
    // 告诉 Qt 这个类实现了 node_editor_interface 接口
    Q_INTERFACES(node_editor_interface)
    // 插件元数据，IID 必须与接口定义中一致
    Q_PLUGIN_METADATA(IID NodeEditorInterface_iid)

public:
    explicit node_plugin(QObject* parent = nullptr);
    virtual ~node_plugin();

    // --- 接口实现 ---
    
    /**
     * @brief 创建并返回节点编辑器的窗口部件
     * @param parent 父容器指针，通常由主程序的 QStackedWidget 传入
     */
    QWidget* create_editor(QWidget* parent = nullptr) override;

    /** @brief 获取插件在 UI 上显示的友好名称 */
    QString get_plugin_name() const override;

    /** @brief 获取插件在 VS Code 风格侧边栏显示的图标 */
    QIcon get_icon() const override;

    /** @brief 执行场景序列化，保存为 JSON */
    void save_to_file(const QString& path) override;

    /** @brief 执行场景反序列化，从 JSON 加载 */
    void load_from_file(const QString& path) override;

private:
    // 每个插件实例持有自己的 view 指针，杜绝使用 static 变量
    // 这允许主程序同时加载并打开多个独立的节点编辑页面
    dong_dong_widgets::node_editor::NodeEditor* _view;
};

#endif // NODE_PLUGIN_H