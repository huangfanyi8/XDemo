
#ifndef SWITCH_BUTTON__INTERFACE_H
#define SWITCH_BUTTON__INTERFACE_H

class QWidget;
class QString;
class QIcon;

class node_editor_interface
{
public:
    virtual ~node_editor_interface() {}

    // 核心 UI 生成
    virtual QWidget* create_editor(QWidget* parent = nullptr) = 0;

    // 插件信息
    virtual QString get_plugin_name() const = 0;
    virtual QIcon get_icon() const = 0;

    // 持久化
    virtual void save_to_file(const QString& path) = 0;
    virtual void load_from_file(const QString& path) = 0;
};

// 确保字符串唯一且两边一致
#define NodeEditorInterface_iid "com.dongdong.NodeEditorInterface/1.0"
Q_DECLARE_INTERFACE(node_editor_interface, NodeEditorInterface_iid)

#endif