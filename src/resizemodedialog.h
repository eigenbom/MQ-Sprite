#ifndef RESIZEMODEDIALOG_H
#define RESIZEMODEDIALOG_H

#include <QDialog>
#include <QLineEdit>

namespace Ui {
class ResizeModeDialog;
}

class ResizeModeDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ResizeModeDialog(QWidget *parent = nullptr);
    ~ResizeModeDialog();
    QLineEdit *mLineEditWidth, *mLineEditHeight;
    QLineEdit *mLineEditOffsetX, *mLineEditOffsetY;

private:
    Ui::ResizeModeDialog *ui;

};

#endif // RESIZEMODEDIALOG_H
