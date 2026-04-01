#ifndef XDEMO_MODERNTABWIDGET_H
#define XDEMO_MODERNTABWIDGET_H

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
#include <QTabBar>

// --- 1. 自定义标签栏：处理动画与核心绘图 ---
class EdgeTabBar
    : public QTabBar
 {
    Q_OBJECT
public:
    explicit EdgeTabBar(QWidget *parent = nullptr)
        : QTabBar(parent)
    {
        //允许标签关闭
        this->setTabsClosable(true);
        this->setMovable(true); // 开启拖拽
        // 设置 Fixed 策略，否则动画会被布局拉伸挤压
        this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    // 动画驱动变量
    int _animWidth = 0;
    int _animIndex = -1;

    // --- 添加标签动画 ---
    void addTabWithAnim(const QString& text) {
        _animIndex = this->count();
        _animWidth = 0;
        this->addTab(text);

        auto *ani = new QVariantAnimation(this);
        ani->setDuration(500);
        ani->setStartValue(0);
        ani->setEndValue(200);
        ani->setEasingCurve(QEasingCurve::OutBack);

        connect(ani, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            _animWidth = value.toInt();
            this->updateGeometry(); // 通知布局改变
            this->update();         // 触发重绘
        });

        connect(ani, &QVariantAnimation::finished, this, [this] {
            _animIndex = -1;
            this->updateGeometry();
        });
        ani->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // --- 删除标签动画 ---
    void removeTabWithAnim(int index) {
        if (_animIndex != -1 || index < 0 || index >= count()) return;

        _animIndex = index;
        _animWidth = 200;

        auto *ani = new QVariantAnimation(this);
        ani->setDuration(300);
        ani->setStartValue(200);
        ani->setEndValue(0);
        ani->setEasingCurve(QEasingCurve::InCubic);

        connect(ani, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            _animWidth = value.toInt();
            this->updateGeometry();
            this->update();
        });

        connect(ani, &QVariantAnimation::finished, this, [this, index] {
            _animIndex = -1;
            this->removeTab(index); // 物理删除标签
            this->updateGeometry();
            emit tabRemovedSync(index); // 通知外部同步删除 StackWidget 页面
        });
        ani->start(QAbstractAnimation::DeleteWhenStopped);
    }

signals:
    void tabRemovedSync(int index);

protected:
    // 每一个标签的理想大小
    QSize tabSizeHint(int index) const override {
        if (index == _animIndex) return {_animWidth, 70};
        // 最后一个标签留出 10px 间隙给分割线/按钮
        return (index == count() - 1) ? QSize(210, 70) : QSize(200, 70);
    }

    // 整个组件的大小：由所有标签宽度累加决定
    QSize sizeHint() const override {
        int totalW = 0;
        for (int i = 0; i < count(); ++i) {
            totalW += tabSizeHint(i).width();
        }
        return {totalW, 70};
    }

    QSize minimumTabSizeHint(int index) const override {
        return {0, 70};
    }

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 背景色
        painter.fillRect(this->rect(), QColor(0xCDCDCD));

        // 底部分割线
        painter.setPen(QPen(QColor(0xCFD4DB), 1));
        painter.drawLine(0, height() - 1, width(), height() - 1);

        for (int i = 0; i < count(); i++) {
            QStyleOptionTab opt;
            this->initStyleOption(&opt, i);

            // 关键：使用 opt.rect 以适配拖拽时的坐标位移
            QRectF _current_rect = opt.rect;

            // 动画期间或正常状态的右侧微调
            if (i == count() - 1 && _animIndex == -1) {
                _current_rect.setRight(_current_rect.right() - 10);
            }

            QRectF _draw_rect = _current_rect.adjusted(2, 6, -2, -1);
            if (_draw_rect.width() <= 0) continue;

            if (opt.state & QStyle::State_Selected) {
                // 选中态：圆角路径绘制
                constexpr qreal r = 12.0;
                QPainterPath path;
                path.moveTo(_draw_rect.left() - r, _draw_rect.bottom());
                path.quadTo(_draw_rect.left(), _draw_rect.bottom(), _draw_rect.left(), _draw_rect.bottom() - r);
                path.lineTo(_draw_rect.left(), _draw_rect.top() + r);
                path.quadTo(_draw_rect.left(), _draw_rect.top(), _draw_rect.left() + r, _draw_rect.top());
                path.lineTo(_draw_rect.right() - r, _draw_rect.top());
                path.quadTo(_draw_rect.right(), _draw_rect.top(), _draw_rect.right(), _draw_rect.top() + r);
                path.lineTo(_draw_rect.right(), _draw_rect.bottom() - r);
                path.quadTo(_draw_rect.right(), _draw_rect.bottom(), _draw_rect.right() + r, _draw_rect.bottom());

                painter.fillPath(path, QBrush(QColor(0xF7F7F7)));
                painter.setPen(Qt::black);
                painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
                painter.drawText(_draw_rect, Qt::AlignCenter, opt.text);
            } else {
                // 非选中态
                painter.setPen(Qt::black);
                painter.setFont(QFont("Microsoft YaHei", 9));
                painter.drawText(_draw_rect, Qt::AlignCenter, opt.text);

                // 分割线绘制
                if (i != this->currentIndex() - 1 || i == this->count() - 1) {
                    painter.setPen(QPen(Qt::black, 1));
                    const qreal lineX = _draw_rect.right() + 4;
                    painter.drawLine(QPointF(lineX, _draw_rect.center().y() - 14),
                                     QPointF(lineX, _draw_rect.center().y() + 14));
                }
            }
        }
    }
};

// 容器组件：协调 TabBar 与 StackedWidget
class EdgeTabWidget
    : public QWidget
{
    Q_OBJECT
public:
    explicit EdgeTabWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        this->resize(1000, 800);
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // 顶部工具栏
        auto *_top_widget = new QWidget;
        _top_widget->setFixedHeight(78);
        _top_widget->setStyleSheet("background-color: #CDCDCD;");
        auto *_top_layout = new QHBoxLayout(_top_widget);
        _top_layout->setContentsMargins(15, 4, 15, 0);
        _top_layout->setSpacing(0);

        _tab_bar = new EdgeTabBar;
        _add_button = new QPushButton("+");
        _add_button->setFixedSize(65, 65);
        _add_button->setStyleSheet("QPushButton { border: none; font-size: 22px; color: #777; }"
                                   "QPushButton:hover { background: #D1D6DE; border-radius: 17px; }");

        _top_layout->addWidget(_tab_bar);
        _top_layout->addWidget(_add_button);
        _top_layout->addStretch();

        _stack = new QStackedWidget;
        layout->addWidget(_top_widget);
        layout->addWidget(_stack);

        // 信号关联
        connect(_add_button, &QPushButton::clicked, [this] { this->addPage(); });
        connect(_tab_bar, &QTabBar::currentChanged, _stack, &QStackedWidget::setCurrentIndex);

        // 删除逻辑：先动效，后物理删除内容
        connect(_tab_bar, &QTabBar::tabCloseRequested, _tab_bar, &EdgeTabBar::removeTabWithAnim);
        connect(_tab_bar, &EdgeTabBar::tabRemovedSync, [this](int index) {
            QWidget *w = _stack->widget(index);
            if (w) {
                _stack->removeWidget(w);
                w->deleteLater();
            }
        });

        addPage(); // 初始化第一页
    }

    void addPage() {
        int index = _stack->addWidget(new QWidget);
        _tab_bar->addTabWithAnim(QString("Tab %1").arg(index + 1));
        _tab_bar->setCurrentIndex(index);
    }

private:
    EdgeTabBar *_tab_bar;
    QStackedWidget *_stack;
    QPushButton *_add_button;
};


#endif //XDEMO_MODERNTABWIDGET_H