#include <QApplication>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        // 1. 创建场景和视图
        QGraphicsScene *scene = new QGraphicsScene(this);
        QGraphicsView *view = new QGraphicsView(scene, this);
        view->setRenderHint(QPainter::Antialiasing);
        view->setSceneRect(0, 0, 400, 300);
        setCentralWidget(view);

        // 2. 创建部件
        QPushButton *button = new QPushButton("Add Text");
        QLineEdit *lineEdit = new QLineEdit;
        lineEdit->setPlaceholderText("Output will appear here");
        lineEdit->setReadOnly(true);

        // 3. 将部件添加到场景（使用便捷函数）
        QGraphicsProxyWidget *proxyButton = scene->addWidget(button);
        QGraphicsProxyWidget *proxyLineEdit = scene->addWidget(lineEdit);

        // 4. 设置位置
        proxyButton->setPos(50, 50);
        proxyLineEdit->setPos(50, 100);
        proxyLineEdit->resize(200, 30);  // 调整代理大小

        // 5. 连接信号
        connect(button, &QPushButton::clicked, this, [lineEdit](){
            lineEdit->setText("Button clicked!");
        });
    }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}



#include "module1.moc"