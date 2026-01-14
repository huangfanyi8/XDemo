#include "main_window.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

dark_window::dark_window(QWidget *parent) : QMainWindow(parent)
{
    // 1. 必须设置无边框，但要保留系统按钮功能以支持 Aero Snap
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);

    setStyleSheet("background-color: #1e1e1e; border: 1px solid #333;");
    resize(800, 600);
}

#ifdef Q_OS_WIN
bool dark_window::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    MSG* msg = static_cast<MSG*>(message);

    if (msg->message == WM_NCHITTEST) {
        // 1. 获取鼠标在屏幕上的绝对位置
        int xPos = GET_X_LPARAM(msg->lParam);
        int yPos = GET_Y_LPARAM(msg->lParam);

        // 2. 将屏幕坐标转换为相对于窗口的坐标
        QPoint pos = mapFromGlobal(QPoint(xPos, yPos));
        int x = pos.x();
        int y = pos.y();
        int w = width();
        int h = height();

        // 3. 判断鼠标是否在边缘
        bool onLeft   = x < m_borderWidth;
        bool onRight  = x > w - m_borderWidth;
        bool onTop    = y < m_borderWidth;
        bool onBottom = y > h - m_borderWidth;

        // 4. 关键：向 Windows 返回对应的命中代码 (Hit-Test Codes)
        // Windows 会根据这些返回值自动处理拉伸，绝对不抖动
        if (onTop && onLeft)      *result = HTTOPLEFT;
        else if (onTop && onRight)     *result = HTTOPRIGHT;
        else if (onBottom && onLeft)   *result = HTBOTTOMLEFT;
        else if (onBottom && onRight)  *result = HTBOTTOMRIGHT;
        else if (onLeft)               *result = HTLEFT;
        else if (onRight)              *result = HTRIGHT;
        else if (onTop)                *result = HTTOP;
        else if (onBottom)             *result = HTBOTTOM;
        else if (y < 40)               *result = HTCAPTION; // 标题栏拖拽
        else return QMainWindow::nativeEvent(eventType, message, result);

        return true; // 表示该消息已处理，不再向下传递
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif