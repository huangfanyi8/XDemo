
#include "../include/PluginInterfaceBase.h"
#include <QPushButton>
#include <QObject>

class ButtonPluginV2 : public QObject, public PluginInterfaceBase
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterfaceBase_iid FILE "ButtonV2.json")
    Q_INTERFACES(PluginInterfaceBase)

public:
    QString name() const override { return "Button Switch"; }
    QString version() const override { return "2.0.0"; }
    QStringList history() const override { return {"1.5.0", "1.2.0"}; }

    QWidget* create_widget(QWidget *parent) override
    {
        QPushButton *btn = new QPushButton("增强型开关", parent);
        btn->setCheckable(true);
        btn->setStyleSheet(
            "QPushButton { background-color: #7C3AED; color: white; border-radius: 18px; padding: 12px 18px; }"
            "QPushButton:checked { background-color: #F97316; }"
        );
        return btn;
    }
};

#include "ButtonV2.moc"
