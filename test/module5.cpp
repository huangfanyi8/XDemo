#include <QApplication>
#include <QAbstractScrollArea>
#include <QScrollArea>
#include <QScrollBar>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEvent>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDebug>
#include <QtGlobal>

/**
 * @brief 前台显示用的幽灵滚动条。
 */
class ghost_overlay_scroll_bar : public QScrollBar
{
public:
    static constexpr int bar_width = 20;
    static constexpr int bar_margin = 3;
    static constexpr int handle_min_height = 36;
    static constexpr int border_radius = 6;
    static constexpr int fade_duration_ms = 160;

public:
    explicit ghost_overlay_scroll_bar(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QScrollBar(orientation, parent)
        , m_opacity_effect(new QGraphicsOpacityEffect(this))
        , m_fade_animation(new QPropertyAnimation(m_opacity_effect, "opacity", this))
    {
        setGraphicsEffect(m_opacity_effect);
        m_opacity_effect->setOpacity(0.0);

        m_fade_animation->setDuration(fade_duration_ms);

        connect(m_fade_animation, &QPropertyAnimation::finished, this, [this]() {
            if (m_target_opacity <= 0.0) {
                hide();
            }
        });

        setMouseTracking(true);
        hide();

        apply_style();
    }

    void fade_to(qreal opacity)
    {
        m_target_opacity = opacity;

        if (opacity > 0.0 && !isVisible()) {
            show();
        }

        m_fade_animation->stop();
        m_fade_animation->setStartValue(m_opacity_effect->opacity());
        m_fade_animation->setEndValue(opacity);
        m_fade_animation->start();
    }

private:
    void apply_style()
    {
        setStyleSheet(QString(R"(
        QScrollBar:vertical {
            background: rgba(120, 120, 120, 55);
            width: %1px;
            margin: %2px;
            border: none;
            border-radius: %3px;
        }

        QScrollBar::handle:vertical {
            background: rgba(90, 90, 90, 190);
            min-height: %4px;
            border-radius: %5px;
            margin: 2px;
        }

        QScrollBar::handle:vertical:hover {
            background: rgba(70, 70, 70, 220);
        }

        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical {
            background: transparent;
            border: none;
        }

        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            background: transparent;
            height: 0px;
            border: none;
        }
    )")
        .arg(bar_width)
        .arg(bar_margin)
        .arg(border_radius)
        .arg(handle_min_height)
        .arg(border_radius - 2));
    }

private:
    QGraphicsOpacityEffect* m_opacity_effect = nullptr;
    QPropertyAnimation* m_fade_animation = nullptr;
    qreal m_target_opacity = 0.0;
};

/**
 * @brief 最简版幽灵滚动条控制器。
 */
class ghost_scroll_controller : public QObject
{
public:
    explicit ghost_scroll_controller(QAbstractScrollArea* area)
        : QObject(area)
        , m_area(area)
        , m_real_bar(area->verticalScrollBar())
        , m_fake_bar(new ghost_overlay_scroll_bar(Qt::Vertical, area->viewport()))
    {
        Q_ASSERT(m_area);
        Q_ASSERT(m_real_bar);
        Q_ASSERT(m_fake_bar);

        // 隐藏真实滚动条，避免内容区尺寸跳动
        m_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        // 让 viewport 和假滚动条都能把事件先交给控制器
        m_area->viewport()->setMouseTracking(true);
        m_fake_bar->setMouseTracking(true);
        m_area->viewport()->installEventFilter(this);
        m_fake_bar->installEventFilter(this);

        // 真实滚动条变化 -> 同步给假滚动条
        connect(m_real_bar, &QScrollBar::valueChanged, this, [this]() {
            sync_bar();
        });

        connect(m_real_bar, &QScrollBar::rangeChanged, this, [this]() {
            sync_bar();
        });

        // 假滚动条变化 -> 写回真实滚动条
        connect(m_fake_bar, &QScrollBar::valueChanged, this, [this](int value) {
            if (m_real_bar->value() != value) {
                m_real_bar->setValue(value);
            }
        });

        sync_bar();
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_area->viewport()) {
            switch (event->type()) {
            case QEvent::Enter:
            case QEvent::MouseMove:
            case QEvent::Wheel:
            {
                if (m_real_bar->maximum() > m_real_bar->minimum()) {
                    QWidget* vp = m_area->viewport();

                    m_fake_bar->setGeometry(
                        vp->width() - ghost_overlay_scroll_bar::bar_width - ghost_overlay_scroll_bar::bar_margin,
                        ghost_overlay_scroll_bar::bar_margin,
                        ghost_overlay_scroll_bar::bar_width,
                        vp->height() - ghost_overlay_scroll_bar::bar_margin * 2
                    );

                    m_fake_bar->raise();
                    m_fake_bar->fade_to(0.95);
                }
                break;
            }

            case QEvent::Resize:
            {
                QWidget* vp = m_area->viewport();

                m_fake_bar->setGeometry(
                    vp->width() - ghost_overlay_scroll_bar::bar_width - ghost_overlay_scroll_bar::bar_margin,
                    ghost_overlay_scroll_bar::bar_margin,
                    ghost_overlay_scroll_bar::bar_width,
                    vp->height() - ghost_overlay_scroll_bar::bar_margin * 2
                );

                m_fake_bar->raise();
                break;
            }

            case QEvent::Leave:
            {
                if (!m_fake_bar->underMouse()) {
                    m_fake_bar->fade_to(0.0);
                }
                break;
            }

            default:
                break;
            }
        }

        if (watched == m_fake_bar) {
            switch (event->type()) {
            case QEvent::Enter:
            {
                if (m_real_bar->maximum() > m_real_bar->minimum()) {
                    QWidget* vp = m_area->viewport();

                    m_fake_bar->setGeometry(
                        vp->width() - ghost_overlay_scroll_bar::bar_width - ghost_overlay_scroll_bar::bar_margin,
                        ghost_overlay_scroll_bar::bar_margin,
                        ghost_overlay_scroll_bar::bar_width,
                        vp->height() - ghost_overlay_scroll_bar::bar_margin * 2
                    );

                    m_fake_bar->raise();
                    m_fake_bar->fade_to(0.95);
                }
                break;
            }

            case QEvent::Leave:
            {
                if (!m_area->viewport()->underMouse()) {
                    m_fake_bar->fade_to(0.0);
                }
                break;
            }

            default:
                break;
            }
        }

        return QObject::eventFilter(watched, event);
    }

private:
    void sync_bar()
    {
        if (m_real_bar->maximum() <= m_real_bar->minimum()) {
            m_fake_bar->hide();
            return;
        }

        m_fake_bar->setRange(m_real_bar->minimum(), m_real_bar->maximum());
        m_fake_bar->setPageStep(m_real_bar->pageStep());
        m_fake_bar->setSingleStep(m_real_bar->singleStep());

        if (m_fake_bar->value() != m_real_bar->value()) {
            m_fake_bar->setValue(m_real_bar->value());
        }

        QWidget* vp = m_area->viewport();

        m_fake_bar->setGeometry(
            vp->width() - ghost_overlay_scroll_bar::bar_width - ghost_overlay_scroll_bar::bar_margin,
            ghost_overlay_scroll_bar::bar_margin,
            ghost_overlay_scroll_bar::bar_width,
            vp->height() - ghost_overlay_scroll_bar::bar_margin * 2
        );

        m_fake_bar->raise();
    }

private:
    QAbstractScrollArea* m_area = nullptr;
    QScrollBar* m_real_bar = nullptr;
    ghost_overlay_scroll_bar* m_fake_bar = nullptr;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 构造一个很长的内容区域，确保可以滚动
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    for (int i = 0; i < 80; ++i) {
        auto* item = new QFrame;
        item->setFrameShape(QFrame::StyledPanel);
        item->setStyleSheet(R"(
            QFrame {
                background: white;
                border: 1px solid #d0d0d0;
                border-radius: 6px;
            }
        )");

        auto* item_layout = new QVBoxLayout(item);
        item_layout->addWidget(new QLabel(QString("测试项 %1").arg(i + 1)));
        item_layout->addWidget(new QLabel("把鼠标移入内容区，右侧应出现幽灵滚动条。"));
        layout->addWidget(item);
    }

    layout->addStretch();

    auto* area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setWidget(content);
    area->resize(520, 420);
    area->setWindowTitle("ghost_scroll_controller 测试");

    // 挂上代理滚动条
    new ghost_scroll_controller(area);

    area->show();
    return app.exec();
}