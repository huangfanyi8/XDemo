#include <QApplication>
#include "dark_window.h"

int main(int argc, char *argv[]) {
    // 强制同步渲染，解决向左缩放时右侧内容的“飘移”
    qputenv("QT_NO_ASYNCHRONOUS_WINDOWS_EVENTS", "1");
    // 禁用线程化渲染，让所有绘图在主线程同步完成
    qputenv("QSG_RENDER_LOOP", "basic");

    QApplication a(argc, argv);
    dark_window w;
    w.show();
    return a.exec();
}