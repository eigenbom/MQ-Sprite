#ifndef DRAWINGTOOLS_H
#define DRAWINGTOOLS_H

#include <QWidget>

namespace Ui {
class DrawingTools;
}

class DrawingTools : public QWidget
{
    Q_OBJECT

public:
    explicit DrawingTools(QWidget *parent = nullptr);
    ~DrawingTools();

private:
    Ui::DrawingTools *ui;
};

#endif // DRAWINGTOOLS_H
