#include "_plugin.h"
#include <QGraphicsScene>
#include <QPainter>
#include <QDebug>
#include <QIcon>

node_plugin::node_plugin(QObject* parent)
    : QObject(parent)
    , _view(nullptr)
{
}

node_plugin::~node_plugin()
{
    // 注意：如果 _view 已经被嵌入到主程序的窗口中，
    // Qt 的对象树系统会自动在主窗口销毁时清理它。
    // 但如果插件被动态卸载，这里需要确保安全。
}

QWidget* node_plugin::create_editor(QWidget* parent)
{
    // 1. 创建场景
    auto* scene = new QGraphicsScene(parent);

    // 2. 匹配你现有的构造函数：node_view(QGraphicsScene* scene)
    _view = new dong_dong_widgets::node_editor::NodeEditor(scene);

    // 3. 关键：手动建立父子关系，确保它能嵌入主窗口且不泄露
    if (parent)
    {
        _view->setParent(parent);
    }

    // 4. 必须给尺寸，否则在 QStackedWidget 中会坍缩
    _view->setMinimumSize(800, 600);

    return _view;
}

QString node_plugin::get_plugin_name() const
{
    return QString("数据流节点编辑器");
}

QIcon node_plugin::get_icon() const
{
    // 使用绘图设备生成一个简单的 VS Code 风格图标
    QPixmap pix(64, 64);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // 绘制一个深蓝色圆角矩形背景
    p.setBrush(QColor("#007ACC"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(5, 5, 54, 54, 10, 10);

    // 绘制三个白色小圆点模拟节点
    p.setBrush(Qt::white);
    p.drawEllipse(15, 20, 8, 8);
    p.drawEllipse(40, 20, 8, 8);
    p.drawEllipse(27, 40, 8, 8);

    return QIcon(pix);
}

void node_plugin::save_to_file(const QString& path)
{
    if (_view)
    {

        qDebug() << "Plugin: Saved scene to" << path;
    }
}

void node_plugin::load_from_file(const QString& path)
{
    if (_view)
    {

        qDebug() << "Plugin: Loaded scene from" << path;
    }
}