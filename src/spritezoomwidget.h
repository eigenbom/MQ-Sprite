#ifndef SPRITEZOOMWIDGET_H
#define SPRITEZOOMWIDGET_H

#include <QWidget>

namespace Ui {
class SpriteZoomWidget;
}

class SpriteZoomWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SpriteZoomWidget(QWidget *parent = nullptr);
    ~SpriteZoomWidget();

private:
    Ui::SpriteZoomWidget *ui;
};

#endif // SPRITEZOOMWIDGET_H
