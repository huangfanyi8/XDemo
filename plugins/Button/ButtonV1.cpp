
#include "../include/PluginInterfaceBase.h"
#include <QPushButton>
#include <QObject>

class ButtonPluginV1 : public QObject, public PluginInterfaceBase
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterfaceBase_iid FILE "ButtonV1.json")
    Q_INTERFACES(PluginInterfaceBase)

public:
    QString name() const override { return "Button Switch"; }
    QString version() const override { return "1.0.0"; }
    QStringList history() const override { return {"0.9.0", "0.8.0"}; }

    QWidget* create_widget(QWidget *parent) override
    {
        QPushButton *btn = new QPushButton("高级开关", parent);
        btn->setCheckable(true);
        btn->setStyleSheet(
            "QPushButton { background-color: #2196F3; color: white; border-radius: 15px; }"
            "QPushButton:checked { background-color: #FF5722; }"
        );
        return btn;
    }
};

#include "ButtonV1.moc"