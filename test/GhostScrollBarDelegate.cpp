#include <QApplication>
#include <QAbstractScrollArea>
#include <QScrollArea>
#include <QScrollBar>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEvent>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDebug>
#include <QtGlobal>




int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 构造一个很长的内容区域，确保可以滚动
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    for (int i = 0; i < 80; ++i) {
        auto* item = new QFrame;
        item->setFrameShape(QFrame::StyledPanel);
        item->setStyleSheet(R"(
            QFrame {
                background: white;
                border: 1px solid #d0d0d0;
                border-radius: 6px;
            }
        )");

        auto* item_layout = new QVBoxLayout(item);
        item_layout->addWidget(new QLabel(QString("测试项 %1").arg(i + 1)));
        item_layout->addWidget(new QLabel("把鼠标移入内容区，右侧应出现幽灵滚动条。"));
        layout->addWidget(item);
    }

    layout->addStretch();

    auto* area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setWidget(content);
    area->resize(520, 420);
    area->setWindowTitle("ghost_scroll_controller 测试");

    // 挂上代理滚动条
    new ghost_scroll_controller(area);

    area->show();

    // 当前程序所在目录，比 QDir::currentPath() 更稳定
    const QString app_dir = QCoreApplication::applicationDirPath();

    qDebug() << "Application directory:" << QDir::toNativeSeparators(app_dir);

    const QString project_root = find_project_root(app_dir);

    if (project_root.isEmpty())
    {
        qDebug() << "Project root not found.";
    }
    else
    {
        qDebug() << "Project root found:" << QDir::toNativeSeparators(project_root);
    }
    return app.exec();
}