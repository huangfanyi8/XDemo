// PluginManagementView.cpp
#include "PluginVersionManager.h"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStyle>
#include <QVBoxLayout>


namespace xdemo::plugin_management
{

class PluginDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PluginDetailWidget(QWidget *parent = nullptr)
        : QWidget(parent),
          m_manager(nullptr),
          m_name_label(nullptr),
          m_summary_label(nullptr),
          m_version_list(nullptr),
          m_rename_button(nullptr),
          m_add_version_button(nullptr),
          m_switch_button(nullptr),
          m_delete_button(nullptr)
    {
        setObjectName(QStringLiteral("pluginDetailWidget"));
        setAttribute(Qt::WA_StyledBackground, true);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(16);

        m_name_label = new QLabel(this);
        m_name_label->setObjectName(QStringLiteral("detailTitle"));
        layout->addWidget(m_name_label);

        m_summary_label = new QLabel(this);
        m_summary_label->setObjectName(QStringLiteral("mutedLabel"));
        layout->addWidget(m_summary_label);

        auto *title = new QLabel(tr("版本列表"), this);
        title->setObjectName(QStringLiteral("sectionTitle"));
        layout->addWidget(title);

        m_version_list = new QListWidget(this);
        layout->addWidget(m_version_list, 1);

        auto *button_layout = new QVBoxLayout;
        button_layout->setSpacing(10);

        m_rename_button = _create_button(tr("重命名插件"));
        m_add_version_button = _create_button(tr("添加版本"));
        m_switch_button = _create_button(tr("切换版本"));
        m_delete_button = _create_button(tr("删除版本"));

        button_layout->addWidget(m_rename_button);
        button_layout->addWidget(m_add_version_button);
        button_layout->addWidget(m_switch_button);
        button_layout->addWidget(m_delete_button);
        layout->addLayout(button_layout);
        layout->addStretch();

        connect(m_rename_button, &QPushButton::clicked, this, &PluginDetailWidget::on_rename_plugin);
        connect(m_add_version_button, &QPushButton::clicked, this, &PluginDetailWidget::on_add_version);
        connect(m_switch_button, &QPushButton::clicked, this, &PluginDetailWidget::on_switch_version);
        connect(m_delete_button, &QPushButton::clicked, this, &PluginDetailWidget::on_delete_version);

        clear();
    }

    void set_plugin(PluginVersionManager *manager, const QString &plugin_id)
    {
        m_manager = manager;
        m_plugin_id = plugin_id;
        _set_buttons_enabled(true);
        _update_ui();
    }

    void clear()
    {
        m_manager = nullptr;
        m_plugin_id.clear();
        m_name_label->setText(tr("未选择插件"));
        m_summary_label->setText(tr("从左侧插件列表中选择一个插件。"));
        m_version_list->clear();
        _set_buttons_enabled(false);
    }

signals:
    void plugin_data_changed(const QString &plugin_id);

private slots:
    void on_rename_plugin()
    {
        const PluginInfo *plugin = _current_plugin();
        if (plugin == nullptr)
        {
            return;
        }

        bool ok = false;
        const QString new_name = QInputDialog::getText(this,
                                                       tr("重命名插件"),
                                                       tr("请输入新的插件名称："),
                                                       QLineEdit::Normal,
                                                       plugin->name,
                                                       &ok).trimmed();
        if (!ok || new_name.isEmpty() || new_name == plugin->name)
        {
            return;
        }

        QString error_message;
        if (!m_manager->rename_plugin(m_plugin_id, new_name, &error_message))
        {
            QMessageBox::information(this, tr("提示"), error_message);
            return;
        }

        _update_ui();
        emit plugin_data_changed(m_plugin_id);
    }

void on_add_version()
{
    if (_current_plugin() == nullptr)
    {
        return;
    }

    QDialog create_dialog(this);
    create_dialog.setWindowTitle(tr("添加版本"));

    auto *form_layout = new QFormLayout(&create_dialog);
    auto *version_edit = new QLineEdit(&create_dialog);
    auto *load_button = new QPushButton(tr("加载插件"), &create_dialog);
    load_button->setIcon(QIcon::fromTheme("document-open", QIcon(":/icons/open.png"))); // 标准图标，备选资源

    auto *layout = new QHBoxLayout();
    layout->addWidget(version_edit);
    layout->addWidget(load_button);
    form_layout->addRow(tr("版本号："), layout);

    auto *button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                            Qt::Horizontal,
                                            &create_dialog);
    form_layout->addRow(button_box);
    create_dialog.setLayout(form_layout);   // 原代码遗漏，必须设置

    // 加载插件按钮逻辑
    connect(load_button, &QPushButton::clicked, &create_dialog, [&]() {
        // 弹出文件选择对话框，过滤动态库文件
        QString file_path = QFileDialog::getOpenFileName(
            &create_dialog,
            tr("选择Qt插件"),
            QString(),
            tr("Qt插件 (*.so *.dll *.dylib);;所有文件 (*)"));
        if (file_path.isEmpty())
            return;

        // 校验插件
        QPluginLoader loader(file_path);
        if (!loader.load())
        {
            QMessageBox::warning(&create_dialog,
                                 tr("校验失败"),
                                 tr("无法加载插件：\n%1").arg(loader.errorString()));
            return;
        }

        // 获取插件元数据（例如 JSON 格式）
        QJsonObject meta = loader.metaData();
        if (meta.isEmpty())
        {
            QMessageBox::warning(&create_dialog,
                                 tr("校验失败"),
                                 tr("插件缺少元数据，不是有效的Qt插件。"));
            loader.unload();
            return;
        }

        // 从元数据中提取版本号（示例：假设存在 "version" 字段）
        QString version;
        if (meta.contains("version"))
        {
            version = meta["version"].toString();
        }
        else
        {
            // 若没有元数据版本，可从文件名或其他方式提取
            QFileInfo info(file_path);
            version = info.baseName();   // 仅作示例，实际可自定义规则
        }

        // 填入版本号输入框
        version_edit->setText(version);
        loader.unload();   // 校验完成，卸载插件

        QMessageBox::information(&create_dialog,
                                 tr("校验成功"),
                                 tr("插件有效，版本号已自动填入。"));
    });

    connect(button_box, &QDialogButtonBox::accepted, &create_dialog, &QDialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, &create_dialog, &QDialog::reject);

    if (create_dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    _update_ui();
    emit plugin_data_changed(m_plugin_id);
}

    void on_switch_version()
    {
        if (_current_plugin() == nullptr)
        {
            return;
        }

        const QString version = _selected_version();
        if (version.isEmpty())
        {
            QMessageBox::information(this, tr("提示"), tr("请先选择一个版本。"));
            return;
        }

        QString error_message;
        if (!m_manager->switch_version(m_plugin_id, version, &error_message))
        {
            QMessageBox::information(this, tr("提示"), error_message);
            return;
        }

        _update_ui();
        emit plugin_data_changed(m_plugin_id);
    }

    void on_delete_version()
    {
        if (_current_plugin() == nullptr)
        {
            return;
        }

        const QString version = _selected_version();
        if (version.isEmpty())
        {
            QMessageBox::information(this, tr("提示"), tr("请先选择一个版本。"));
            return;
        }

        QString error_message;
        if (!m_manager->delete_version(m_plugin_id, version, &error_message))
        {
            QMessageBox::information(this, tr("提示"), error_message);
            return;
        }

        _update_ui();
        emit plugin_data_changed(m_plugin_id);
    }

private:
    QPushButton *_create_button(const QString &text)
    {
        auto *button = new QPushButton(text, this);
        button->setObjectName(QStringLiteral("actionButton"));
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(40);
        return button;
    }

    void _set_buttons_enabled(bool enabled)
    {
        m_rename_button->setEnabled(enabled);
        m_add_version_button->setEnabled(enabled);
        m_switch_button->setEnabled(enabled);
        m_delete_button->setEnabled(enabled);
    }

    const PluginInfo *_current_plugin() const
    {
        if (m_manager == nullptr || m_plugin_id.isEmpty())
        {
            return nullptr;
        }

        return m_manager->find_plugin(m_plugin_id);
    }

    // 列表文本会附加“当前”，真实版本号仍然保存在 UserRole。
    QString _selected_version() const
    {
        QListWidgetItem *item = m_version_list->currentItem();
        if (item == nullptr)
        {
            return QString();
        }

        return item->data(Qt::UserRole).toString();
    }

    // 列表始终显示所有版本，不再因为切换而把旧版本“顶掉”。
    void _refresh_version_list(const PluginInfo &plugin)
    {
        m_version_list->clear();

        for (const PluginVersionInfo &version_info : plugin.versions)
        {
            QString text = version_info.version;
            if (version_info.version == plugin.current_version)
            {
                text += tr(" (当前)");
            }

            auto *item = new QListWidgetItem(text, m_version_list);
            item->setData(Qt::UserRole, version_info.version);

            QString tooltip = version_info.description;
            if (!version_info.binary_file_path.isEmpty())
            {
                if (!tooltip.isEmpty())
                {
                    tooltip += QLatin1Char('\n');
                }

                tooltip += tr("库文件: %1").arg(version_info.binary_file_path);
            }

            if (!tooltip.isEmpty())
            {
                item->setToolTip(tooltip);
            }
        }
    }

    void _update_ui()
    {
        const PluginInfo *plugin = _current_plugin();
        if (plugin == nullptr)
        {
            clear();
            return;
        }

        int binary_count = 0;
        for (const PluginVersionInfo &version_info : plugin->versions)
        {
            if (version_info.has_binary())
            {
                ++binary_count;
            }
        }

        m_name_label->setText(plugin->name);
        m_summary_label->setText(tr("当前版本: %1 | 已收录: %2 | 可预览: %3")
                                     .arg(plugin->current_version,
                                          QString::number(static_cast<int>(plugin->versions.size())),
                                          QString::number(binary_count)));

        _refresh_version_list(*plugin);
    }

    PluginVersionManager *m_manager;
    QString m_plugin_id;
    QLabel *m_name_label;
    QLabel *m_summary_label;
    QListWidget *m_version_list;
    QPushButton *m_rename_button;
    QPushButton *m_add_version_button;
    QPushButton *m_switch_button;
    QPushButton *m_delete_button;
};

class PreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget *parent = nullptr)
        : QWidget(parent),
          m_manager(nullptr),
          m_preview_container(nullptr),
          m_current_preview(nullptr)
    {
        setObjectName(QStringLiteral("previewWidget"));
        setAttribute(Qt::WA_StyledBackground, true);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(16);

        auto *title = new QLabel(tr("控件预览"), this);
        title->setObjectName(QStringLiteral("sectionTitle"));
        layout->addWidget(title);

        m_preview_container = new QWidget(this);
        m_preview_container->setObjectName(QStringLiteral("previewContainer"));
        m_preview_container->setAttribute(Qt::WA_StyledBackground, true);

        auto *container_layout = new QVBoxLayout(m_preview_container);
        container_layout->setContentsMargins(20, 20, 20, 20);
        container_layout->setAlignment(Qt::AlignCenter);
        layout->addWidget(m_preview_container);
    }

    void set_plugin(PluginVersionManager *manager, const QString &plugin_id)
    {
        m_manager = manager;
        m_plugin_id = plugin_id;
        refresh_preview();
    }

    void clear_preview()
    {
        m_plugin_id.clear();
        _clear_container();
        _show_message(tr("无预览控件"));
    }

    void refresh_preview()
    {
        _clear_container();

        if (m_manager == nullptr || m_plugin_id.isEmpty())
        {
            _show_message(tr("无预览控件"));
            return;
        }

        QString error_message;
        m_current_preview = m_manager->create_preview_widget(m_plugin_id, m_preview_container, &error_message);
        if (m_current_preview == nullptr)
        {
            _show_message(error_message.isEmpty() ? tr("当前版本无预览控件。") : error_message);
            return;
        }

        m_preview_container->layout()->addWidget(m_current_preview);
        m_current_preview->show();
    }

private:
    void _clear_container()
    {
        m_current_preview = nullptr;

        QLayout *layout = m_preview_container->layout();
        QLayoutItem *child = nullptr;
        while ((child = layout->takeAt(0)) != nullptr)
        {
            if (QWidget *widget = child->widget())
            {
                widget->deleteLater();
            }

            delete child;
        }
    }

    void _show_message(const QString &message)
    {
        auto *label = new QLabel(message, m_preview_container);
        label->setAlignment(Qt::AlignCenter);
        m_preview_container->layout()->addWidget(label);
    }

    PluginVersionManager *m_manager;
    QString m_plugin_id;
    QWidget *m_preview_container;
    QWidget *m_current_preview;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        _setup_ui();
        _apply_style();
        _load_project_plugins();
        _refresh_plugin_list();
    }

private slots:
    void on_create_plugin()
    {
        QDialog create_dialog(this);
        create_dialog.setWindowTitle(tr("创建插件"));

        auto *form_layout = new QFormLayout(&create_dialog);
        auto *plugin_name_edit = new QLineEdit(&create_dialog);
        auto *default_version_edit = new QLineEdit(QStringLiteral("1.0.0"), &create_dialog);
        form_layout->addRow(tr("插件名称："), plugin_name_edit);
        form_layout->addRow(tr("默认版本："), default_version_edit);

        auto *button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                Qt::Horizontal,
                                                &create_dialog);
        form_layout->addRow(button_box);

        connect(button_box, &QDialogButtonBox::accepted, &create_dialog, &QDialog::accept);
        connect(button_box, &QDialogButtonBox::rejected, &create_dialog, &QDialog::reject);

        if (create_dialog.exec() != QDialog::Accepted)
        {
            return;
        }

        const QString plugin_name = plugin_name_edit->text().trimmed();
        const QString default_version = default_version_edit->text().trimmed();
        if (plugin_name.isEmpty() || default_version.isEmpty())
        {
            QMessageBox::warning(this, tr("创建失败"), tr("插件名称和默认版本不能为空。"));
            return;
        }

        QString plugin_id;
        QString error_message;
        if (!m_plugin_manager.create_plugin(plugin_name, default_version, &plugin_id, &error_message))
        {
            QMessageBox::warning(this, tr("创建失败"), error_message);
            return;
        }

        _refresh_plugin_list(plugin_id);
    }

    void on_plugin_selected(int row)
    {
        const PluginInfo *plugin = m_plugin_manager.plugin_at(row);
        if (plugin == nullptr)
        {
            m_detail_widget->clear();
            m_preview_widget->clear_preview();
            return;
        }

        m_detail_widget->set_plugin(&m_plugin_manager, plugin->id);
        m_preview_widget->set_plugin(&m_plugin_manager, plugin->id);
    }

    void on_plugin_data_changed(const QString &plugin_id)
    {
        _refresh_plugin_list(plugin_id);
    }

private:
    void _setup_ui()
    {
        setWindowTitle(tr("插件管理器"));
        resize(1080, 720);

        auto *central_widget = new QWidget(this);
        central_widget->setObjectName(QStringLiteral("centralWidget"));
        central_widget->setAttribute(Qt::WA_StyledBackground, true);
        setCentralWidget(central_widget);

        auto *main_layout = new QHBoxLayout(central_widget);
        main_layout->setContentsMargins(18, 18, 18, 18);
        main_layout->setSpacing(18);

        auto *left_panel = new QWidget(this);
        left_panel->setObjectName(QStringLiteral("sidebarPanel"));
        left_panel->setAttribute(Qt::WA_StyledBackground, true);

        auto *left_layout = new QVBoxLayout(left_panel);
        left_layout->setContentsMargins(22, 22, 22, 22);
        left_layout->setSpacing(14);

        auto *list_title = new QLabel(tr("插件列表"), left_panel);
        list_title->setObjectName(QStringLiteral("sectionTitle"));
        left_layout->addWidget(list_title);

        m_plugin_list = new QListWidget(left_panel);
        left_layout->addWidget(m_plugin_list);

        m_create_plugin_button = new QPushButton(tr("创建插件"), left_panel);
        m_create_plugin_button->setObjectName(QStringLiteral("actionButton"));
        m_create_plugin_button->setCursor(Qt::PointingHandCursor);
        m_create_plugin_button->setMinimumHeight(40);
        m_create_plugin_button->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
        left_layout->addWidget(m_create_plugin_button);

        auto *right_panel = new QWidget(this);
        auto *right_layout = new QVBoxLayout(right_panel);
        right_layout->setContentsMargins(0, 0, 0, 0);

        m_detail_widget = new PluginDetailWidget(this);
        m_preview_widget = new PreviewWidget(this);
        m_detail_widget->setMinimumWidth(320);
        m_detail_widget->setMaximumWidth(360);
        m_preview_widget->setMinimumWidth(420);

        auto *right_splitter = new QSplitter(Qt::Horizontal, this);
        right_splitter->setChildrenCollapsible(false);
        right_splitter->addWidget(m_detail_widget);
        right_splitter->addWidget(m_preview_widget);
        right_splitter->setStretchFactor(0, 1);
        right_splitter->setStretchFactor(1, 4);
        right_layout->addWidget(right_splitter);

        auto *main_splitter = new QSplitter(Qt::Horizontal, this);
        main_splitter->setChildrenCollapsible(false);
        main_splitter->addWidget(left_panel);
        main_splitter->addWidget(right_panel);
        main_splitter->setStretchFactor(0, 1);
        main_splitter->setStretchFactor(1, 2);
        main_layout->addWidget(main_splitter);

        connect(m_plugin_list, &QListWidget::currentRowChanged, this, &MainWindow::on_plugin_selected);
        connect(m_create_plugin_button, &QPushButton::clicked, this, &MainWindow::on_create_plugin);
        connect(m_detail_widget, &PluginDetailWidget::plugin_data_changed, this, &MainWindow::on_plugin_data_changed);
    }

    void _apply_style()
    {
        const QString style = QStringLiteral(R"(
        QMainWindow {
            background-color: #0b1120;
        }
        QWidget#centralWidget {
            background-color: #0b1120;
        }
        QWidget#sidebarPanel,
        QWidget#previewWidget {
            background-color: #111827;
            border: 1px solid #1f2937;
            border-radius: 18px;
        }
        QWidget#pluginDetailWidget {
            background-color: #111827;
            border: none;
            border-radius: 18px;
        }
        QListWidget {
            background-color: #0f172a;
            border: 1px solid #243041;
            border-radius: 14px;
            padding: 8px;
            outline: none;
            color: #dbe7f5;
        }
        QListWidget::item {
            background-color: #172033;
            border-radius: 12px;
            margin: 4px;
            padding: 10px 12px;
            color: #dbe7f5;
        }
        QListWidget::item:selected,
        QListWidget::item:selected:focus,
        QListWidget::item:selected:!focus {
            background-color: #2563eb;
            color: #f8fafc;
            border: none;
        }
        QWidget#previewContainer {
            background-color: #0f172a;
            border: 1px solid #243041;
            border-radius: 16px;
        }
        QPushButton#actionButton {
            background-color: #2563eb;
            border: none;
            border-radius: 12px;
            padding: 10px 18px;
            color: #f8fafc;
            font-weight: 600;
        }
        QPushButton#actionButton:hover {
            background-color: #3b82f6;
        }
        QPushButton#actionButton:pressed {
            background-color: #1d4ed8;
        }
        QLabel {
            color: #cbd5e1;
        }
        QLabel#sectionTitle {
            color: #f8fafc;
            font-size: 14px;
            font-weight: 600;
        }
        QLabel#detailTitle {
            color: #f8fafc;
            font-size: 22px;
            font-weight: 700;
        }
        QLabel#mutedLabel {
            color: #94a3b8;
            font-size: 13px;
        }
        QSplitter::handle {
            background-color: #1f2937;
            width: 6px;
            margin: 6px 0;
            border-radius: 3px;
        }

    )");

        setStyleSheet(style);
    }

    void _load_project_plugins()
    {
        const QString metadata_root = QDir::cleanPath(QDir(QDir::currentPath()).absoluteFilePath(QStringLiteral("plugins")));
        const QString binary_root = QDir::cleanPath(QDir(QDir::currentPath()).absoluteFilePath(QStringLiteral("bin/plugins")));
        m_plugin_manager.load_from_directories(metadata_root, binary_root);
    }

    void _refresh_plugin_list(const QString &selected_plugin_id = QString())
    {
        QString target_plugin_id = selected_plugin_id;
        if (target_plugin_id.isEmpty())
        {
            QListWidgetItem *current_item = m_plugin_list->currentItem();
            if (current_item != nullptr)
            {
                target_plugin_id = current_item->data(Qt::UserRole).toString();
            }
        }

        m_plugin_list->clear();

        int target_row = -1;
        const std::vector<PluginInfo> &plugins = m_plugin_manager.plugins();
        for (std::size_t index = 0; index < plugins.size(); ++index)
        {
            const PluginInfo &plugin = plugins[index];
            auto *item = new QListWidgetItem(QStringLiteral("%1  (%2 / %3 个版本)")
                                                 .arg(plugin.name,
                                                      plugin.current_version,
                                                      QString::number(static_cast<int>(plugin.versions.size()))),
                                             m_plugin_list);
            item->setData(Qt::UserRole, plugin.id);

            if (plugin.id == target_plugin_id)
            {
                target_row = static_cast<int>(index);
            }
        }

        if (m_plugin_list->count() == 0)
        {
            m_detail_widget->clear();
            m_preview_widget->clear_preview();
            return;
        }

        m_plugin_list->setCurrentRow(target_row < 0 ? 0 : target_row);
    }

    PluginVersionManager m_plugin_manager;
    QListWidget *m_plugin_list;
    QPushButton *m_create_plugin_button;
    PluginDetailWidget *m_detail_widget;
    PreviewWidget *m_preview_widget;
};

} // namespace xdemo::plugin_management


int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    xdemo::plugin_management::MainWindow window;
    window.show();
    return application.exec();
}

#include "PluginManagementView.moc"
