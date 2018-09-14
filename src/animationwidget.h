#ifndef ANIMATIONWIDGET_H
#define ANIMATIONWIDGET_H

#include <QWidget>

namespace Ui {
class AnimationWidget;
}

class AnimationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnimationWidget(QWidget *parent = nullptr);
    ~AnimationWidget();

private:
    Ui::AnimationWidget *ui;
};

#endif // ANIMATIONWIDGET_H
