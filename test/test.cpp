#include <QApplication>
#include <QPushButton>
#include <QPointer>
#include <QWidget>

#include "PluginBuilder.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QWidget w;
    w.resize(400, 300);

    auto *button = new QPushButton(QStringLiteral("打开插件构建器"), &w);
    button->move(120, 100);

    QPointer<PluginBuilderView> builder_window;

    QObject::connect(button, &QPushButton::clicked, &w, [&]() {
        if (builder_window.isNull())
        {
            builder_window = new PluginBuilderView;
            builder_window->setAttribute(Qt::WA_DeleteOnClose);
        }

        builder_window->show();
        builder_window->raise();
        builder_window->activateWindow();
    });

    w.show();

    return a.exec();
}