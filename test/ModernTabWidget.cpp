#include"ModernTabWidget.h"

int main(int argc, char *argv[])
{

    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication a(argc, argv);
    EdgeTabWidget w;
    w.show();
    return a.exec();
}