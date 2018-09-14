#include "optionswidget.h"
#include "ui_optionswidget.h"

OptionsWidget::OptionsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OptionsWidget)
{
    ui->setupUi(this);
}

OptionsWidget::~OptionsWidget()
{
    delete ui;
}
