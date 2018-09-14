#ifndef PROPERTIESWIDGET_H
#define PROPERTIESWIDGET_H

#include <QWidget>

namespace Ui {
class PropertiesWidget;
}

class PropertiesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PropertiesWidget(QWidget *parent = nullptr);
    ~PropertiesWidget();

private:
    Ui::PropertiesWidget *ui;
};

#endif // PROPERTIESWIDGET_H
