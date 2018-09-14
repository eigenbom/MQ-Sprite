#include "propertieswidget.h"
#include "ui_propertieswidget.h"

PropertiesWidget::PropertiesWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PropertiesWidget)
{
    ui->setupUi(this);
}

PropertiesWidget::~PropertiesWidget()
{
    delete ui;
}
