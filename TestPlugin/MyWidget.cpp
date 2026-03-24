//
// Created by Lenovo on 2026/3/24.
//

#include "MyWidget.h"

#include "MyWidget.h"

#include <QLabel>
#include <QVBoxLayout>

MyWidget::MyWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(QStringLiteral("Hello Plugin"), this);
    layout->addWidget(label);
    setLayout(layout);
}