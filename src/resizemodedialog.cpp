#include "resizemodedialog.h"
#include "ui_resizemodedialog.h"
#include <QIntValidator>

ResizeModeDialog::ResizeModeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ResizeModeDialog)
{
    ui->setupUi(this);

    mLineEditWidth = findChild<QLineEdit*>("lineEditWidth");
    mLineEditHeight = findChild<QLineEdit*>("lineEditHeight");
    mLineEditOffsetX = findChild<QLineEdit*>("lineEditOffsetX");
    mLineEditOffsetY = findChild<QLineEdit*>("lineEditOffsetY");

    mLineEditWidth->setValidator(new QIntValidator(0,1024));
    mLineEditHeight->setValidator(new QIntValidator(0,1024));
    mLineEditOffsetX->setValidator(new QIntValidator(-1024,1024));
    mLineEditOffsetY->setValidator(new QIntValidator(-1024,1024));
}

ResizeModeDialog::~ResizeModeDialog()
{
    delete ui;
}
