#include"NodeEditor.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication a(argc, argv);
    NodeEditor::MainWindow w;
    w.show();
    return a.exec();
}