#include "MyWidget.h"
#include <QVBoxLayout>
#include <QLabel>

MyWidget::MyWidget(QWidget *parent)
    : QWidget(parent)
{
    setup_ui();
}

QString MyWidget::name() const
{
    return "MyWidget";
}

QString MyWidget::version() const
{
    return "1.0.0";
}

QStringList MyWidget::history() const
{
    return QStringList() << "1.0.0";
}

QWidget* MyWidget::create_widget(QWidget *parent)
{
    return new MyWidget(parent);
}

void MyWidget::setup_ui()
{
    auto *layout = new QVBoxLayout(this);
    const QString label_text = QString::fromUtf8(u8"MyWidget - /U8FD9/U662F/U4E00/U4E2A/U7EC4/U4EF6");
    auto *label = new QLabel(label_text, this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}
