#include <QApplication>
#include <QDrag>
#include <QFrame>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMath>

/**
 * @brief 左侧组件箱
 *
 * 作用：
 * 1. 展示可拖拽的组件类型
 * 2. 在鼠标拖动达到阈值后发起拖拽
 *
 * 拖拽时并不是把真正的 QWidget 拖出去，
 * 而是只传递一个“控件类型字符串”。
 */
class ToolboxListWidget : public QListWidget
{
public:
    explicit ToolboxListWidget(QWidget* parent = nullptr)
        : QListWidget(parent)
    {
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSpacing(4);
    }

protected:
    /**
     * @brief 记录鼠标按下位置
     *
     * 后面在 mouseMoveEvent 中会通过当前点与起点的距离，
     * 判断用户到底是“点击”还是“拖拽”。
     */
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
            m_drag_start_pos = event->pos();

        QListWidget::mousePressEvent(event);
    }

    /**
     * @brief 鼠标移动时，如果达到拖拽阈值，就发起拖拽
     */
    void mouseMoveEvent(QMouseEvent* event) override
    {
        // 不是左键按住拖动，交给父类处理
        if (!(event->buttons() & Qt::LeftButton))
        {
            QListWidget::mouseMoveEvent(event);
            return;
        }

        // 鼠标移动距离过小，说明还只是普通点击，不发起拖拽
        if ((event->pos() - m_drag_start_pos).manhattanLength()
            < QApplication::startDragDistance())
        {
            QListWidget::mouseMoveEvent(event);
            return;
        }

        // 获取当前选中的项
        QListWidgetItem* item = currentItem();
        if (!item)
        {
            QListWidget::mouseMoveEvent(event);
            return;
        }

        start_drag(item);
    }

private:
    /**
     * @brief 发起拖拽
     * @param item 当前被拖拽的组件项
     *
     * 我们把组件类型写入自定义 MIME 数据：
     * application/x-designer-widget
     *
     * 右侧设计画布收到后，根据类型创建真正的控件。
     */
    void start_drag(QListWidgetItem* item)
    {
        if (!item)
            return;

        const QString widget_type = item->data(Qt::UserRole).toString();
        if (widget_type.isEmpty())
            return;

        // 1. 创建 MIME 数据对象
        auto* mime_data = new QMimeData;
        mime_data->setData("application/x-designer-widget", widget_type.toUtf8());

        // 2. 创建拖拽对象
        auto* drag = new QDrag(this);
        drag->setMimeData(mime_data);

        // 3. 给拖拽过程设置一个简易预览图
        QPixmap pixmap(120, 32);
        pixmap.fill(Qt::transparent);

        {
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setBrush(QColor(245, 245, 245));
            painter.setPen(QPen(Qt::gray, 1));
            painter.drawRoundedRect(pixmap.rect().adjusted(1, 1, -2, -2), 6, 6);
            painter.setPen(Qt::black);
            painter.drawText(pixmap.rect(), Qt::AlignCenter, item->text());
        }

        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

        // 4. 执行拖拽
        drag->exec(Qt::CopyAction);
    }

private:
    QPoint m_drag_start_pos;
};

/**
 * @brief 右侧设计画布
 *
 * 作用：
 * 1. 接收左侧拖过来的组件类型
 * 2. 在场景中创建真正的 QWidget
 * 3. 通过 QGraphicsProxyWidget 放入 QGraphicsScene
 * 4. 让这些代理图元可选中、可移动
 */
class DesignView : public QGraphicsView
{
public:
    explicit DesignView(QWidget* parent = nullptr)
        : QGraphicsView(parent)
        , m_scene(new QGraphicsScene(this))
    {
        setScene(m_scene);

        // 必须开启，否则收不到拖拽事件
        setAcceptDrops(true);

        setRenderHint(QPainter::Antialiasing, true);
        setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

        /**
         * 重点：
         * 不要使用 RubberBandDrag。
         *
         * RubberBandDrag 是视图自身的框选模式，
         * 可能会干扰我们对单个代理图元的拖动体验。
         */
        setDragMode(QGraphicsView::NoDrag);

        m_scene->setSceneRect(0, 0, 1600, 1000);

        setBackgroundBrush(QColor(250, 250, 250));
        setFrameShape(QFrame::StyledPanel);
    }

protected:
    /**
     * @brief 绘制背景网格
     *
     * 只是为了让画布看起来更像“设计器”。
     */
    void drawBackground(QPainter* painter, const QRectF& rect) override
    {
        QGraphicsView::drawBackground(painter, rect);

        constexpr int grid_size = 20;

        const qreal left = std::floor(rect.left() / grid_size) * grid_size;
        const qreal top = std::floor(rect.top() / grid_size) * grid_size;

        QVarLengthArray<QLineF, 256> lines;

        for (qreal x = left; x < rect.right(); x += grid_size)
            lines.append(QLineF(x, rect.top(), x, rect.bottom()));

        for (qreal y = top; y < rect.bottom(); y += grid_size)
            lines.append(QLineF(rect.left(), y, rect.right(), y));

        painter->setPen(QPen(QColor(230, 230, 230), 1));
        painter->drawLines(lines.data(), lines.size());
    }

    /**
     * @brief 拖拽物第一次进入视图时触发
     *
     * 这里只判断是不是我们认识的 MIME 类型。
     * 如果接受，后续才会继续收到 dragMoveEvent / dropEvent。
     */
    void dragEnterEvent(QDragEnterEvent* event) override
    {
        if (event->mimeData()->hasFormat("application/x-designer-widget"))
            event->acceptProposedAction();
        else
            event->ignore();
    }

    /**
     * @brief 拖拽物在视图内部移动时持续触发
     */
    void dragMoveEvent(QDragMoveEvent* event) override
    {
        if (event->mimeData()->hasFormat("application/x-designer-widget"))
            event->acceptProposedAction();
        else
            event->ignore();
    }

    /**
     * @brief 在视图中松手时触发
     *
     * 这里才真正读取拖拽数据，并创建控件。
     */
    void dropEvent(QDropEvent* event) override
    {
        if (!event->mimeData()->hasFormat("application/x-designer-widget"))
        {
            event->ignore();
            return;
        }

        const QString widget_type =
            QString::fromUtf8(event->mimeData()->data("application/x-designer-widget"));

        // 将视图坐标转换为场景坐标
        QPointF scene_pos = mapToScene(event->pos());

        // 做一个简单网格吸附，方便放置
        scene_pos = snap_to_grid(scene_pos, 20.0);

        create_widget_item(widget_type, scene_pos);

        event->acceptProposedAction();
    }

private:
    /**
     * @brief 坐标吸附到网格
     */
    static QPointF snap_to_grid(const QPointF& pos, qreal grid_size)
    {
        const qreal x = std::round(pos.x() / grid_size) * grid_size;
        const qreal y = std::round(pos.y() / grid_size) * grid_size;
        return QPointF(x, y);
    }

    /**
     * @brief 根据类型创建真实 QWidget，并放入场景
     * @param widget_type 控件类型字符串
     * @param scene_pos   在场景中的放置位置
     *
     * 关键点：
     * 真实 QWidget 要设置 WA_TransparentForMouseEvents，
     * 这样它不会抢走鼠标事件，鼠标事件会交给外层的 QGraphicsProxyWidget。
     *
     * 只有这样，proxy 的 ItemIsMovable 才更容易生效。
     */
    void create_widget_item(const QString& widget_type, const QPointF& scene_pos)
    {
        QWidget* widget = nullptr;

        if (widget_type == "QPushButton")
        {
            auto* button = new QPushButton("按钮");
            button->setMinimumSize(100, 36);

            // 设计态下不让真实按钮自己处理鼠标
            button->setAttribute(Qt::WA_TransparentForMouseEvents, true);

            widget = button;
        }
        else if (widget_type == "QLabel")
        {
            auto* label = new QLabel("标签");
            label->setAlignment(Qt::AlignCenter);
            label->setMinimumSize(100, 36);
            label->setStyleSheet(
                "QLabel {"
                "  background: white;"
                "  border: 1px solid #BFBFBF;"
                "  border-radius: 4px;"
                "}");
            label->setAttribute(Qt::WA_TransparentForMouseEvents, true);

            widget = label;
        }
        else if (widget_type == "QLineEdit")
        {
            auto* line_edit = new QLineEdit;
            line_edit->setPlaceholderText("请输入内容");
            line_edit->setMinimumSize(160, 36);

            // 设计态下不进入编辑状态，不抢鼠标
            line_edit->setAttribute(Qt::WA_TransparentForMouseEvents, true);

            widget = line_edit;
        }
        else
        {
            return;
        }

        /**
         * addWidget 会返回 QGraphicsProxyWidget*
         * 它是 QWidget 在场景中的代理图元。
         */
        auto* proxy = m_scene->addWidget(widget);

        // 允许选中
        proxy->setFlag(QGraphicsItem::ItemIsSelectable, true);

        // 允许移动
        proxy->setFlag(QGraphicsItem::ItemIsMovable, true);

        // 发送几何变化通知，后面做属性面板/对齐线时有用
        proxy->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

        // 允许接收焦点
        proxy->setFlag(QGraphicsItem::ItemIsFocusable, true);

        // 只接受左键操作
        proxy->setAcceptedMouseButtons(Qt::LeftButton);

        // 设置位置
        proxy->setPos(scene_pos);

        // 提高一点层级，避免重叠时体验奇怪
        proxy->setZValue(1.0);
    }

private:
    QGraphicsScene* m_scene = nullptr;
};

/**
 * @brief 主窗口
 *
 * 左侧：组件箱
 * 右侧：设计画布
 */
class PluginBuilder : public QMainWindow
{
public:
    explicit PluginBuilder(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
        auto* central = new QWidget(this);
        setCentralWidget(central);

        auto* main_layout = new QHBoxLayout(central);
        main_layout->setContentsMargins(8, 8, 8, 8);
        main_layout->setSpacing(8);

        // =========================
        // 左侧组件箱区域
        // =========================
        auto* left_panel = new QWidget(this);
        left_panel->setFixedWidth(190);

        auto* left_layout = new QVBoxLayout(left_panel);
        left_layout->setContentsMargins(0, 0, 0, 0);
        left_layout->setSpacing(6);

        auto* title = new QLabel("组件箱");
        title->setFixedHeight(32);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet(
            "QLabel {"
            "  background: #EAEAEA;"
            "  border: 1px solid #D0D0D0;"
            "  font-weight: bold;"
            "}");

        auto* toolbox = new ToolboxListWidget(this);
        toolbox->setStyleSheet(
            "QListWidget {"
            "  background: white;"
            "  border: 1px solid #D0D0D0;"
            "}"
            "QListWidget::item {"
            "  height: 32px;"
            "  padding-left: 8px;"
            "}"
            "QListWidget::item:selected {"
            "  background: #DCEBFF;"
            "  color: black;"
            "}");

        add_toolbox_item(toolbox, "按钮", "QPushButton");
        add_toolbox_item(toolbox, "标签", "QLabel");
        add_toolbox_item(toolbox, "单行输入框", "QLineEdit");

        left_layout->addWidget(title);
        left_layout->addWidget(toolbox);

        // =========================
        // 右侧设计画布区域
        // =========================
        auto* design_view = new DesignView(this);

        main_layout->addWidget(left_panel);
        main_layout->addWidget(design_view, 1);

        resize(1200, 760);
        setWindowTitle("Qt Designer Demo - QListWidget 拖拽到 QGraphicsProxyWidget");
    }

private:
    /**
     * @brief 向左侧组件箱添加一项
     * @param list 组件箱
     * @param text 显示名称
     * @param type 内部类型字符串
     */
    static void add_toolbox_item(QListWidget* list, const QString& text, const QString& type)
    {
        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, type);
        list->addItem(item);
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    PluginBuilder window;
    window.show();

    return app.exec();
}