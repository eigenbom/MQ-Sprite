#include "spritezoomwidget.h"
#include "ui_spritezoomwidget.h"

SpriteZoomWidget::SpriteZoomWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SpriteZoomWidget)
{
    ui->setupUi(this);
}

SpriteZoomWidget::~SpriteZoomWidget()
{
    delete ui;
}
