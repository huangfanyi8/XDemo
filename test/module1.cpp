#include <QApplication>
#include <QListView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QMouseEvent>

#include <QApplication>
#include <QListView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QMouseEvent>

class RoundedSideBar : public QListView
{
    Q_OBJECT
public:
    RoundedSideBar(QWidget *parent = nullptr)
        : QListView(parent)
    {
        setMouseTracking(true);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        setSpacing(4); // 行间距略大

        model = new QStandardItemModel(this);
        setModel(model);

        delegate = new SideBarDelegate(this);
        setItemDelegate(delegate);

        connect(selectionModel(), &QItemSelectionModel::currentChanged,
                this, &RoundedSideBar::onCurrentChanged);

        initData();

        setStyleSheet("QListView { background: rgba(40,40,40,230); border: none; }");
    }

private:
    class SideBarDelegate : public QStyledItemDelegate
    {
    public:
        SideBarDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent)
        {
            font.setPointSize(12); // 放大文字
            cornerRadius = 10;     // 圆角
        }

        QSize sizeHint(const QStyleOptionViewItem &option,
                       const QModelIndex &index) const override
        {
            Q_UNUSED(option);
            Q_UNUSED(index);
            return QSize(220, 48); // 增大行高
        }

        void paint(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
        {
            painter->save();

            qreal selAnim = index.data(Qt::UserRole).toReal();
            qreal hoverAnim = index.data(Qt::UserRole+1).toReal();
            qreal barHeightRatio = index.data(Qt::UserRole+2).toReal();

            // 背景透明圆角，暗黑风格
            QColor base(40,40,40,180);
            QColor hoverColor(100,100,100,40);  // hover 半透明灰
            int r = base.red() + (hoverColor.red()-base.red())*hoverAnim;
            int g = base.green() + (hoverColor.green()-base.green())*hoverAnim;
            int b = base.blue() + (hoverColor.blue()-base.blue())*hoverAnim;
            int a = base.alpha() + (hoverColor.alpha()-base.alpha())*hoverAnim;

            QRectF rect = option.rect.adjusted(4,2,-4,-2);
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(QColor(r,g,b,a));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(rect, cornerRadius, cornerRadius);

            // 右侧选中竖线动态渐长
            if(barHeightRatio>0){
                qreal barTop = rect.top() + (rect.height()*(1-barHeightRatio))/2;
                qreal barHeight = rect.height() * barHeightRatio;
                QRectF barRect(rect.right()-8, barTop, 8, barHeight);
                painter->fillRect(barRect, QColor(0x0db9d7));
            }

            // 分割线短
            QRectF line(rect.left() + rect.width()*0.2,
                        rect.bottom()-1, rect.width()*0.6, 1);
            painter->fillRect(line, QColor(255,255,255,50));

            // 文字完全居中
            painter->setFont(font);
            painter->setPen(Qt::white);
            painter->drawText(rect, Qt::AlignCenter, index.data(Qt::DisplayRole).toString());

            painter->restore();
        }

        QFont font;
        int cornerRadius;
    };

    void initData()
    {
        QStringList items = {
            "Explorer", "Search", "Source Control", "Run", "Extensions",
            "Settings", "Debug", "Git", "Problems", "Output", "Terminal", "Help"
        };

        for(const QString &t : items){
            QStandardItem *item = new QStandardItem(t);
            item->setData(0.0, Qt::UserRole);   // 选中动画
            item->setData(0.0, Qt::UserRole+1); // hover动画
            item->setData(0.0, Qt::UserRole+2); // 右侧竖线动画
            model->appendRow(item);
        }
    }

protected:
    void mouseMoveEvent(QMouseEvent *event) override
    {
        QModelIndex idx = indexAt(event->pos());
        for(int row=0; row<model->rowCount(); ++row){
            QModelIndex i = model->index(row,0);
            startHover(i, (i==idx)?1.0:0.0);
        }
        QListView::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent *) override
    {
        for(int row=0; row<model->rowCount(); ++row)
            startHover(model->index(row,0),0.0);
    }

private slots:
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
    {
        if(previous.isValid()){
            startSelect(previous,0.0);
            startBar(previous,0.0);
        }
        if(current.isValid()){
            startSelect(current,1.0);
            startBar(current,1.0);
            scrollTo(current, QAbstractItemView::PositionAtCenter);
        }
    }

private:
    void startSelect(const QModelIndex &index, qreal target)
    {
        QVariantAnimation *anim = new QVariantAnimation(this);
        anim->setDuration(200);
        anim->setStartValue(index.data(Qt::UserRole).toReal());
        anim->setEndValue(target);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim,&QVariantAnimation::valueChanged,[this,index](const QVariant &v){
            model->setData(index,v.toReal(),Qt::UserRole);
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void startHover(const QModelIndex &index, qreal target)
    {
        QVariantAnimation *anim = new QVariantAnimation(this);
        anim->setDuration(150);
        anim->setStartValue(index.data(Qt::UserRole+1).toReal());
        anim->setEndValue(target);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim,&QVariantAnimation::valueChanged,[this,index](const QVariant &v){
            model->setData(index,v.toReal(),Qt::UserRole+1);
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void startBar(const QModelIndex &index, qreal target)
    {
        QVariantAnimation *anim = new QVariantAnimation(this);
        anim->setDuration(200);
        anim->setStartValue(index.data(Qt::UserRole+2).toReal());
        anim->setEndValue(target);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim,&QVariantAnimation::valueChanged,[this,index](const QVariant &v){
            model->setData(index,v.toReal(),Qt::UserRole+2);
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QStandardItemModel *model;
    SideBarDelegate *delegate;
};


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    RoundedSideBar w;
    w.show();

    return a.exec();
}

#include "module1.moc"