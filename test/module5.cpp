#include <QApplication>
#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QDebug>

// --------------------- SidebarItemWidget ---------------------
class SidebarItemWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
public:
    SidebarItemWidget(const QIcon &icon, const QString &text, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_iconLabel = new QLabel;
        m_iconLabel->setPixmap(icon.pixmap(20,20));
        m_textLabel = new QLabel(text);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(12,0,12,0);
        layout->setSpacing(10);
        layout->addWidget(m_iconLabel);
        layout->addWidget(m_textLabel);
        layout->addStretch();

        m_normalColor = QColor(30,30,30);
        m_hoverColor  = QColor(60,60,60);
        m_bgColor     = m_normalColor;

        m_hoverAnim = new QPropertyAnimation(this, "backgroundColor");
        m_hoverAnim->setDuration(120);
        m_hoverAnim->setEasingCurve(QEasingCurve::InOutCubic);

        m_textLabel->setStyleSheet("color:#dcdcdc; font-size:14px; font-family:Segoe UI;");
    }

    void setSelected(bool selected)
    {
        m_selected = selected;
        m_hoverAnim->stop();
        m_bgColor = selected ? m_hoverColor : m_normalColor;
        update();
    }

    QColor backgroundColor() const { return m_bgColor; }
    void setBackgroundColor(const QColor &color)
    {
        m_bgColor = color;
        update();
    }

    void setTheme(const QColor &normal, const QColor &hover, const QColor &text)
    {
        m_normalColor = normal;
        m_hoverColor = hover;
        m_textLabel->setStyleSheet(QString("color:%1; font-size:14px; font-family:Segoe UI;")
                                   .arg(text.name()));
        m_bgColor = m_normalColor;
        update();
    }

protected:
    void enterEvent(QEvent *event) override
    {
        if(!m_selected)
        {
            m_hoverAnim->stop();
            m_hoverAnim->setEndValue(m_hoverColor);
            m_hoverAnim->start();
        }
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        if(!m_selected)
        {
            m_hoverAnim->stop();
            m_hoverAnim->setEndValue(m_normalColor);
            m_hoverAnim->start();
        }
        QWidget::leaveEvent(event);
    }

    void paintEvent(QPaintEvent *event) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), m_bgColor);
        QWidget::paintEvent(event);
    }

private:
    QLabel *m_iconLabel;
    QLabel *m_textLabel;
    QColor m_bgColor;
    QColor m_hoverColor;
    QColor m_normalColor;
    QPropertyAnimation *m_hoverAnim;
    bool m_selected = false;
};

// --------------------- MainWindow ---------------------
class MainWindow : public QWidget
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        resize(400, 400);

        sidebar = new QListWidget(this);
        sidebar->setViewMode(QListView::ListMode);
        sidebar->setFlow(QListView::TopToBottom);
        sidebar->setMovement(QListView::Static);
        sidebar->setSpacing(2);
        sidebar->setFixedWidth(180);
        sidebar->setSelectionMode(QAbstractItemView::SingleSelection);
        sidebar->setFrameShape(QFrame::NoFrame);
        sidebar->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        auto addItem = [&](const QString &text, const QIcon &icon)
        {
            auto* item = new QListWidgetItem(sidebar);
            item->setSizeHint(QSize(160, 40));
            auto* widget = new SidebarItemWidget(icon, text);
            sidebar->addItem(item);
            sidebar->setItemWidget(item, widget);
        };

        addItem("主页",   QIcon("D:/c++/Qt/Qt5/dong_dong_widgets/node_editor/resources/main.svg"));
        addItem("时钟",   QIcon("D:/c++/Qt/Qt5/dong_dong_widgets/node_editor/resources/clock.svg"));

        // 选中处理
        connect(sidebar, &QListWidget::currentRowChanged, this, [&](int row){
            for(int i=0; i<sidebar->count(); ++i)
            {
                auto *widget = qobject_cast<SidebarItemWidget*>(sidebar->itemWidget(sidebar->item(i)));
                if(widget) widget->setSelected(i == row);
            }
        });

        // 初始选中第一项
        sidebar->setCurrentRow(0);

        // 主布局
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0,0,0,0);
        layout->addWidget(sidebar);
        layout->addStretch();

        // 默认深色主题
        applyTheme(true);
    }

    void applyTheme(bool dark)
    {
        if(dark)
        {
            sidebar->setStyleSheet("QListWidget{background:#1e1e1e;}");
            for(int i=0;i<sidebar->count();++i)
            {
                auto *w = qobject_cast<SidebarItemWidget*>(sidebar->itemWidget(sidebar->item(i)));
                if(w) w->setTheme(QColor(30,30,30), QColor(60,60,60), QColor(220,220,220));
            }
        }
        else
        {
            sidebar->setStyleSheet("QListWidget{background:#f0f0f0;}");
            for(int i=0;i<sidebar->count();++i)
            {
                auto *w = qobject_cast<SidebarItemWidget*>(sidebar->itemWidget(sidebar->item(i)));
                if(w) w->setTheme(QColor(240,240,240), QColor(200,200,200), QColor(30,30,30));
            }
        }
    }

private:
    QListWidget* sidebar;
};

// --------------------- main ---------------------
int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    return app.exec();
}

#include "module5.moc"
