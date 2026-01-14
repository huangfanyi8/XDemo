#include "dark_window.h"
#include <QPainter>
#include <QGuiApplication>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformnativeinterface.h>
#include<QWindowSystemInterface>
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <QScreen>

#pragma comment(lib, "dwmapi.lib")

dark_window::dark_window(QWidget *parent) : QMainWindow(parent) {
    // 1. 禁用 Qt 的无边框 Hint，改用 CustomizeWindowHint 骗过 DWM
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::CustomizeWindowHint);

    // 2. 彻底禁用背景擦除（防止向左拉伸时的白边）
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);

    // 3. 强制底层句柄创建
    this->winId();

    // 4. 将客户区扩展到整个窗口以开启 DWM 同步渲染
    HWND hwnd = (HWND)this->winId();
    const MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    resize(1000, 700);
}

bool dark_window::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    MSG* msg = reinterpret_cast<MSG*>(message);


    switch (msg->message) {
        case WM_NCCALCSIZE: {
            if (msg->wParam) {
                *result = 0;
                return true;
            }
            break;
        }

        case WM_WINDOWPOSCHANGING: {
            WINDOWPOS* wp = reinterpret_cast<WINDOWPOS*>(msg->lParam);
            // 关键：告诉 Windows 不要尝试移动/拷贝旧位图
            wp->flags |= SWP_NOCOPYBITS;
            break;
        }

        case WM_WINDOWPOSCHANGED: {
            WINDOWPOS* wp = reinterpret_cast<WINDOWPOS*>(msg->lParam);
            if (!(wp->flags & SWP_NOSIZE)) {
                if (QWindow *win = windowHandle()) {
                    // 1. 直接越过 QWidget，通知底层 QWindow 几何尺寸已变
                    // 这会立刻同步物理窗口位置，消除向左拉时的坐标滞后
                    QRect newRect(wp->x, wp->y, wp->cx, wp->cy);

                    // 2. 底层物理同步：强制立即分发几何改变事件
                    // 这是解决抖动的真正“手动挡”操作
                    QWindowSystemInterface::handleGeometryChange(win, newRect);

                    // 3. 强制渲染线程执行同步刷新
                    // 这替代了之前报错的 handle(...) 调用
                    QWindowSystemInterface::flushWindowSystemEvents();
                }
                // 4. 迫使 Windows 立即发送 WM_PAINT 并同步执行绘制
                ::UpdateWindow(msg->hwnd);
            }
            break;
        }

        case WM_ERASEBKGND: {
            *result = 1; // 彻底拦截背景擦除，解决白边闪烁
            return true;
        }
    return QMainWindow::nativeEvent(eventType, message, result);
}

void dark_window::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    // 这里的 rect 已经由于 NCCALCSIZE 包含了整个物理区域
    painter.fillRect(rect(), QColor(30, 30, 30));

    // 绘制模拟标题栏
    painter.fillRect(0, 0, width(), m_titleHeight, QColor(45, 45, 48));
    painter.setPen(Qt::white);
    painter.drawText(QRect(15, 0, width(), m_titleHeight), Qt::AlignLeft | Qt::AlignVCenter, "Deep Sync Frameless Window");
}