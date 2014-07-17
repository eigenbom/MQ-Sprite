#ifndef PALETTEVIEW_H
#define PALETTEVIEW_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>

class PaletteView : public QWidget
{
    Q_OBJECT
public:
    explicit PaletteView(QWidget *parent = 0);
    ~PaletteView();
    void loadPalette(const QString& palette);

protected:
    void mousePressEvent(QMouseEvent* event);
    void resizeEvent(QResizeEvent *);
signals:
    void colourSelected(QColor);

public slots:

private:
    QLabel* mLabel;    
};

#endif // PALETTEVIEW_H
