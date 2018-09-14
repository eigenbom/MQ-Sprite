#include "drawingtools.h"
#include "ui_drawingtools.h"

DrawingTools::DrawingTools(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DrawingTools)
{
    ui->setupUi(this);
}

DrawingTools::~DrawingTools()
{
    delete ui;
}
