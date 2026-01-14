#ifndef DARK_WINDOW_H
#define DARK_WINDOW_H

#include <QMainWindow>
#include <QWindow>

class dark_window : public QMainWindow {
    Q_OBJECT
public:
    explicit dark_window(QWidget *parent = nullptr);

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    void paintEvent(QPaintEvent *event) override;

private:
    const int m_borderWidth = 8;
    const int m_titleHeight = 40;
};

#endif