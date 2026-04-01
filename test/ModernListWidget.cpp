
#include "ModernListWidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QWidget>
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle(QObject::tr("EditableListWidget 示例"));
    window.resize(600, 420);

    auto *layout = new QVBoxLayout(&window);

    auto *list_widget = new Demo::EditableListWidget(&window);
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