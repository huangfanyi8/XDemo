#include <QApplication>
#include <QWidget>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>

class SimpleWidget : public QWidget {
public:
    SimpleWidget() {
        setWindowTitle("右键编辑");
        resize(300, 200);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::RightButton) {
            // 显示编辑框
            QString text = QInputDialog::getText(this, "编辑", "内容:");
            if (!text.isEmpty()) {
                QMessageBox::information(this, "结果", "保存了: " + text);
            }
        }
    }
};

int main(int argc, char *argv[])
{

    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication app(argc, argv);
    SimpleWidget w;
    w.show();
    return app.exec();
}