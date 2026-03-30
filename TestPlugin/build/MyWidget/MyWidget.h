#ifndef MyWidget_H
#define MyWidget_H

#include "PluginInterfaceBase.h"
#include <QWidget>
#include <QString>
#include <QStringList>

class MyWidget : public QWidget, public PluginInterfaceBase
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterfaceBase_iid FILE "metadata.json")
    Q_INTERFACES(PluginInterfaceBase)

public:
    explicit MyWidget(QWidget *parent = nullptr);
    ~MyWidget() override = default;

    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString version() const override;
    [[nodiscard]] QStringList history() const override;

    QWidget* create_widget(QWidget *parent = nullptr) override;

private:
    void setup_ui();
};

#endif // MyWidget_H
