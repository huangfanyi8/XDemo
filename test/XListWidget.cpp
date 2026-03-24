#include <QListWidget>
#include <QHash>

class QKeyEvent;
class QListWidgetItem;

namespace Demo
{

    /**
     * @class EditableListWidget
     * @brief 支持双击重命名、F2重命名、Delete删除的列表控件
     *
     * 该控件基于 QListWidget 封装，仅使用 Qt 自带信号完成逻辑处理。
     *
     * 功能包括：
     * - 双击条目进行重命名
     * - 按 F2 重命名当前条目
     * - 按 Delete 删除当前条目
     * - 删除前弹出确认框
     * - 名称不能为空
     * - 名称不能重复
     */
    class EditableListWidget : public QListWidget
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         * @param parent 父控件指针
         */
        explicit EditableListWidget(QWidget *parent = nullptr);

        /**
         * @brief 析构函数
         */
        ~EditableListWidget() override = default;

        /**
         * @brief 添加一个可编辑条目
         * @param text 条目文本
         */
        void add_editable_item(const QString &text);

        /**
         * @brief 批量添加可编辑条目
         * @param texts 条目文本列表
         */
        void add_editable_items(const QStringList &texts);

    protected:
        /**
         * @brief 键盘按下事件
         * @param event 键盘事件对象
         */
        void keyPressEvent(QKeyEvent *event) override;

    private:

        /**
         * @brief 连接 Qt 自带信号
         */
        void _connect_signals();

        /**
         * @brief 开始编辑当前条目
         */
        void _edit_current_item();

        /**
         * @brief 删除当前条目
         */
        void _remove_current_item();

        /**
         * @brief 判断名称是否重复
         * @param name 待检查名称
         * @param self 当前条目
         * @return 若重复返回 true，否则返回 false
         */
        bool _is_duplicate_name(const QString &name, QListWidgetItem *self) const;

    private:
        /**
         * @brief 保存每个条目的上一次合法名称
         */
        QHash<QListWidgetItem *, QString> m_old_names;

        /**
         * @brief 内部更新保护标记，防止 setText 时重复进入 itemChanged
         */
        bool m_is_internal_change;
    };

} // namespace Demo


#include <QAbstractItemView>
#include <QKeyEvent>
#include <QMessageBox>

namespace Demo
{

EditableListWidget::EditableListWidget(QWidget *parent)
    : QListWidget(parent)
    , m_is_internal_change(false)
{
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    _connect_signals();
}

void EditableListWidget::add_editable_item(const QString &text)
{
    auto *item = new QListWidgetItem(text);
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    addItem(item);
    m_old_names[item] = text;
}

void EditableListWidget::add_editable_items(const QStringList &texts)
{
    for (const QString &text : texts)
        add_editable_item(text);
}

void EditableListWidget::keyPressEvent(QKeyEvent *event)
{
    if (!event)
        {
        QListWidget::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_F2)
    {
        _edit_current_item();
        return;
    }

    if (event->key() == Qt::Key_Delete)
    {
        _remove_current_item();
        return;
    }

    QListWidget::keyPressEvent(event);
}

void EditableListWidget::_connect_signals()
{
    connect(this, &QListWidget::itemDoubleClicked,this,
            [this](QListWidgetItem *item)
            {
                if (!item) return;
                m_old_names[item] = item->text();
            });

    connect(this, &QListWidget::itemChanged,this,
            [this](QListWidgetItem *item)
            {
                if (!item || m_is_internal_change)
                    return;

                //获得纯文本
                const QString new_name = item->text().trimmed();
                const QString old_name = m_old_names.value(item);

                if (new_name.isEmpty())
                {
                    QMessageBox::warning(this,tr("重命名失败"),tr("名称不能为空。"));

                    m_is_internal_change = true;
                    item->setText(old_name);
                    m_is_internal_change = false;
                    return;
                }

                if (_is_duplicate_name(new_name, item)) {
                    QMessageBox::warning(this,tr("重命名失败"),tr("名称不能重复。"));

                    m_is_internal_change = true;
                    item->setText(old_name);
                    m_is_internal_change = false;
                    return;
                }

                //去除掉首尾空格，更新为纯文本
                if (item->text() != new_name)
                {
                    m_is_internal_change = true;
                    item->setText(new_name);
                    m_is_internal_change = false;
                }

                m_old_names[item] = new_name;
            });
}

void EditableListWidget::_edit_current_item()
{
    QListWidgetItem *item = currentItem();
    if (!item) {
        return;
    }

    m_old_names[item] = item->text();
    editItem(item);
}

void EditableListWidget::_remove_current_item()
{
    const QListWidgetItem *item = currentItem();
    if (!item)
        return;

    const QMessageBox::StandardButton reply =
        QMessageBox::question(this,
                              tr("确认删除"),
                              tr("确定要删除 \"%1\" 吗？").arg(item->text()),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;


    QListWidgetItem *removed_item = takeItem(row(item));
    m_old_names.remove(removed_item);
    delete removed_item;
}

bool EditableListWidget::_is_duplicate_name(const QString &name, QListWidgetItem *self) const
{
    if (!self) {
        return false;
    }

    for (int index = 0; index < count(); ++index) {
        QListWidgetItem *item = this->item(index);
        if (!item || item == self) {
            continue;
        }

        if (item->text() == name) {
            return true;
        }
    }

    return false;
}

} // namespace Demo

#include <QApplication>
#include <QVBoxLayout>
#include <QWidget>

#include "XListWidget.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle(QObject::tr("EditableListWidget 示例"));
    window.resize(600, 420);

    QVBoxLayout *layout = new QVBoxLayout(&window);

    Demo::EditableListWidget *list_widget = new Demo::EditableListWidget(&window);
    list_widget->add_editable_items(QStringList()
                                    << QObject::tr("插件管理器")
                                    << QObject::tr("组件面板")
                                    << QObject::tr("属性编辑器")
                                    << QObject::tr("资源浏览器")
                                    << QObject::tr("日志输出"));

    layout->addWidget(list_widget);

    window.show();

    return app.exec();
}