#ifndef OPTIONSWIDGET_H
#define OPTIONSWIDGET_H

#include <QWidget>

namespace Ui {
class OptionsWidget;
}

class OptionsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OptionsWidget(QWidget *parent = nullptr);
    ~OptionsWidget();

private:
    Ui::OptionsWidget *ui;
};

#endif // OPTIONSWIDGET_H
