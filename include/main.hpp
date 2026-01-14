//
// Created by Lenovo on 2025/12/28.
//

#ifndef SWITCH_BUTTON_MAIN_HPP
#define SWITCH_BUTTON_MAIN_HPP
#include <QApplication>
#include <QDebug>
#include <QStackedWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QStyleOptionTab>

namespace dong_dong_widgets
{
    struct _edge_tab_bar
        :QTabBar
    {
        explicit _edge_tab_bar(QWidget *parent = nullptr)
            : QTabBar{parent}
        {
            this->setTabsClosable(true);//允许拥有关闭按钮
            this->setMovable(true);//设置可以移动
        }
    };
}
// --- 1. 自定义标签栏：专注于像素级视觉表现 ---
class QuarkTabBar
    : public QTabBar
{
    Q_OBJECT
public:
    explicit QuarkTabBar(QWidget *parent = nullptr)
        : QTabBar(parent)
    {
        //setExpanding(false);
        this->setTabsClosable(true);//允许拥有关闭按钮
        this->setMovable(true);
    }

    int _width = 0;
    int _index =-1;
    void _add_page(const QString&text)
    {
        _index = this->count();
        // 1. 先把标签加进去
        this->addTab(text);

        // 2. 创建动画
        auto *_animation = new QVariantAnimation(this);
        _animation->setDuration(200);      // 持续时间 0.2秒
        _animation->setStartValue(0);      // 起始宽度
        _animation->setEndValue(210);      // 目标宽度
        _animation->setEasingCurve(QEasingCurve::OutBack); // 关键：弹性曲线

        // 3. 动画每帧更新
        connect(_animation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) {
            _width = value.toInt();
            this->updateGeometry(); // 强制布局重新计算（推开右侧按钮）
                parentWidget()->update(); // 触发父级重绘
            this->update();         // 触发重绘
        });

        connect(_animation, &QVariantAnimation::finished, this, [this]
        {
            _index =-1;
            this->updateGeometry();
        });

        _animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void _remove_page(int index)
    {
        // 防止重复触发动画
        if (this->_index != -1 || index < 0 || index >= count()) return;

        this->_index = index; // 借用这个变量标记正在缩减的索引

        auto *ani = new QVariantAnimation(this);
        ani->setDuration(200); // 删除通常快一点，更干脆
        ani->setStartValue(200);
        ani->setEndValue(0);
        ani->setEasingCurve(QEasingCurve::InCubic); // 逐渐加速消失

        connect(ani, &QVariantAnimation::valueChanged, this, [this](const QVariant &value){
            _width = value.toInt();
            this->updateGeometry(); // 强制推开布局
            this->update();
        });

        connect(ani, &QVariantAnimation::finished, this, [this, index](){
            // 关键顺序：
            // 1. 重置动画状态
            this->_index = -1;
            // 2. 从 TabBar 物理删除（这一步会改变后续所有 tab 的 index）
            this->removeTab(index);
            // 3. 通知外部同步删除 Stack 页面
            emit _remove_stacked_page(index);
            // 4. 最后刷新一次布局
            this->updateGeometry();
        });

        ani->start(QAbstractAnimation::DeleteWhenStopped);
    }

    signals:
    void _remove_stacked_page( int index);
protected:
    // 尺寸控制：最后一个标签预留 10px 绘图空间
    QSize tabSizeHint(const int index) const override
    {
        if (this->_index==index&&_index!=-1)
            return {_width,70};
        if (index == count() - 1)
            return {210, 70};
        return  {200, 70};
    }

    /**
    minimumTabSizeHint(int index) const —— 内部成员的“生存底线”
作用：告诉 TabBar 内部，当空间被压缩（比如窗口变窄）时，标签最小不能低于多少。

如果不写：默认通常是能显示出“...”省略号的最小宽度。

运用场景：

防崩坏：确保动画从 0 开始时，布局不会因为“最小宽度限制”而拒绝缩小。
     */
    [[nodiscard]] QSize minimumTabSizeHint(int index) const override
    {return {0, 48};}
    /*
     *
     * 作用：这是 QWidget 的标准函数。它告诉外部的 Layout（布局管理器）：“我整个 TabBar 组件一共需要多少像素”。
     */
    QSize sizeHint() const override
    {
        int totalW = 0;
        for (int i = 0; i < count(); ++i)
        {
            if (i == _index)
                totalW += _width;
            else
                totalW += tabSizeHint(i).width();
        }
        return {totalW, 70};
    }
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.fillRect(this->rect(),0XCDCDCD);
        // A. 绘制底部贯穿线 (未选中状态的基础连线)
        painter.setPen(QPen(QColor(0xCFD4DB), 1));
        painter.drawLine(0, height() - 1, width(), height() - 1);

        for (int i = 0; i < count(); i++)
        {
            QStyleOptionTab opt;
            this->initStyleOption(&opt, i);

            // 返回的是第 i个标签在整个 TabBar 坐标系中的位置
            QRectF _current_rect = this->tabRect(i);

            if (i == count() - 1)
                _current_rect.setRight(_current_rect.right() - 10);

            // 绘图区域微调：顶部留白 6px，底部留白 1px
            QRectF _draw_rect = _current_rect.adjusted(2, 6, -2, -1);

            if (opt.state & QStyle::State_Selected)
            {
                constexpr qreal r = 12.0;
                // B. 选中态绘制：白色背景 + 全圆角路径
                QPainterPath path;
                // 左下角：反向圆角衔接背景线
                path.moveTo(_draw_rect.left() - r, _draw_rect.bottom());
                path.quadTo(_draw_rect.left(), _draw_rect.bottom(), _draw_rect.left(), _draw_rect.bottom() - r);
                // 左上角：标准圆角
                path.lineTo(_draw_rect.left(), _draw_rect.top() + r);
                path.quadTo(_draw_rect.left(), _draw_rect.top(), _draw_rect.left() + r, _draw_rect.top());
                // 右上角：标准圆角 (满足左右上角圆角要求)
                path.lineTo(_draw_rect.right() - r, _draw_rect.top());
                path.quadTo(_draw_rect.right(), _draw_rect.top(), _draw_rect.right(), _draw_rect.top() + r);
                // 右下角：反向圆角衔接背景线
                path.lineTo(_draw_rect.right(), _draw_rect.bottom() - r);
                path.quadTo(_draw_rect.right(), _draw_rect.bottom(), _draw_rect.right() + r, _draw_rect.bottom());

                painter.fillPath(path, QBrush(0xF7F7F7));

                // 绘制选中文字（加粗）
                painter.setPen(Qt::black);
                painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
                painter.drawText(_draw_rect, Qt::AlignCenter, opt.text);
            }
            else
            {
                // C. 未选中态绘制：文字 + 竖状分割线
                painter.setPen(Qt::black);
                painter.setFont(QFont("Microsoft YaHei", 9));
                painter.drawText(_draw_rect, Qt::AlignCenter, opt.text);

                // 分割线逻辑：右侧非选中标签 或 最后一个标签与按钮之间
                if (i != this->currentIndex() - 1 || i == this->count() - 1)
                {
                    painter.setPen(QPen(Qt::black, 1));
                    const qreal lineX = _draw_rect.right() + 4;
                    const qreal centerY = _draw_rect.center().y();
                    painter.drawLine(QPointF(lineX, centerY - 14), QPointF(lineX, centerY + 14));
                }
            }
        }
    }
};

// --- 2. 封装好的组件容器 ---
class QuarkTabWidget
    : public QWidget
{
public:
    explicit QuarkTabWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        //设置大小
        this->resize(1000,800);
        this->setMinimumSize(1000,800);
        //总布局
        _main_layout = new QVBoxLayout(this);
        _main_layout->setContentsMargins(0, 0, 0, 0);
        _main_layout->setSpacing(0);

        //添加标签上的按钮
        _add_button = new QPushButton("+");
        _add_button->setFixedSize(65, 65);
        _add_button->setStyleSheet(R"(
            QPushButton { border: none; background: transparent; font-size: 22px; color: #777; margin-top: 6px; }
            QPushButton:hover { background: #D1D6DE; border-radius: 17px; }
        )");

        //顶部控件
        auto *_top_widget = new QWidget;
        _top_widget->setFixedHeight(78);
        _top_widget->setStyleSheet("background-color: #CDCDCD;");

        //设置叠层控件用于切换界面
        _stack = new QStackedWidget;
        _stack->setStyleSheet("background-color: white; border-top: 1px solid #CFD4DB;");

        //顶部控件布局
        auto *_top_widget_layout = new QHBoxLayout(_top_widget);
        _top_widget_layout->setContentsMargins(15, 4, 15, 0);
        _top_widget_layout->setSpacing(0);
        _top_widget_layout->addWidget(_tab_bar);
        _top_widget_layout->addWidget(_add_button);
        _top_widget_layout->addStretch();
        _tab_bar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        //主界面布局
        _main_layout->addWidget(_top_widget);
        _main_layout->addWidget(_stack);

        // 信号关联
        QObject::connect(_add_button, &QPushButton::clicked, [this]{ this->_add_page(); });
        QObject::connect(_tab_bar, &QTabBar::currentChanged, _stack, &QStackedWidget::setCurrentIndex);
        QObject::connect(_tab_bar, &QTabBar::tabCloseRequested, [this](const int index){ _tab_bar->_remove_page(index); });
        // 2. 监听动画彻底完成的信号，此时同步操作 StackWidget
        QObject::connect(_tab_bar, &QuarkTabBar::_remove_stacked_page, [this](int index){
            // 获取对应的 Widget
            QWidget *w = _stack->widget(index);
            if (w) {
                _stack->removeWidget(w);
                w->deleteLater(); // 优雅删除内容页
            }
        });
        _init_ui();
    }

    void _init_ui() const
    {
        this->_add_page();
    }

    void _add_page() const
    {
        const int index = _stack->addWidget(new QWidget);
        _tab_bar->_add_page(QString("新标签页 %1").arg(index + 1));
        _tab_bar->setCurrentIndex(index);
    }

    //删除某一页
    void _remove_page(const int index) const
    {
        if (_tab_bar->count() > 1)
        {
            _tab_bar->removeTab(index);
            auto *w = _stack->widget(index);
            _stack->removeWidget(w);
            delete w;
        }
    }

private:
    QVBoxLayout*_main_layout;
    QuarkTabBar *_tab_bar=new QuarkTabBar;
    QStackedWidget *_stack;
    QPushButton *_add_button;
};

#endif //SWITCH_BUTTON_MAIN_HPP