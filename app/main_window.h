#ifndef DARK_WINDOW_H
#define DARK_WINDOW_H

#include <QMainWindow>

class dark_window : public QMainWindow
{
    Q_OBJECT
public:
    explicit dark_window(QWidget *parent = nullptr);

protected:
    // 关键：拦截原生事件
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    const int m_borderWidth = 8; // 响应拉伸的边缘宽度
};

#endif