

#ifndef XDEMO_MODERNLISTWIDGET_H
#define XDEMO_MODERNLISTWIDGET_H
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
#endif //XDEMO_MODERNLISTWIDGET_H