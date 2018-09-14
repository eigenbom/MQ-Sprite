#include "paletteview.h"
#include <QPixmap>
#include <QLabel>
#include <QVBoxLayout>
#include <QDebug>
#include <QMouseEvent>
#include <QResizeEvent>
#include <cmath>

PaletteView::PaletteView(QWidget *parent) :
    QWidget(parent)
{
    // setBackgroundBrush(QBrush(QColor(200,200,200)));

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMinimumSize(50,50);

    setContentsMargins(0,0,0,0);
    setCursor(QCursor(QPixmap::fromImage(QImage(":/icon/icons/tool_pickcolour.png")),8,8));
    mLabel = new QLabel(this);
    QImage img = QImage(140,140,QImage::Format_ARGB32);
    img.fill(0xFFFFFFFF);
    mLabel->setPixmap(QPixmap::fromImage(img));
    mLabel->setScaledContents(true); // false
    mLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // mLabel->setMinimumSize(100,100);
    QVBoxLayout* layout = new QVBoxLayout();    
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    //layout->setStretch(0,1);
    layout->addWidget(mLabel);
    // layout->setStretch(0,1);
    setLayout(layout);
}

PaletteView::~PaletteView(){

}

void PaletteView::loadPalette(const QString& palette){
    QPixmap pixmap = QPixmap(palette).scaled(140,140,Qt::KeepAspectRatio,Qt::FastTransformation); // KeepAspectRatioByExpanding
    if (pixmap.isNull()){
        mLabel = nullptr;
        qWarning() << "Couldn't load default palette";
    }
    else {
         mLabel->setPixmap(pixmap);
    }
}

void PaletteView::mousePressEvent(QMouseEvent* event){
    if (mLabel){
        QPoint pt = mLabel->mapFromParent( event->pos());
        // QPointF pt = this->mapFrom(mLabel, event->pos()); // this->mapToScene(event->pos().x(),event->pos().y());
        int px = pt.x();
        int py = pt.y();

        // qDebug() << px << py << mLabel->pixmap()->width() << mLabel->pixmap()->height();

        if (px < 0 || px >=  mLabel->width() || py < 0 || py >=  mLabel->height()){
            // do nothing
        }
        else {            
            QImage img = mLabel->pixmap()->toImage();

            int w = mLabel->width(), h = mLabel->height();
            float dx = ((float)pt.x())/w;
            float dy = ((float)pt.y())/h;

            // qDebug() << mLabel->contentsRect();
            // qDebug() << pt.x() << pt.y() << dx << dy << dx*img.width() << dy*img.height();
            QRgb rgb = img.pixel(dx*img.width(), dy*img.height());
            // QRgb rgb = img.pixel(pt.x(),pt.y());
            QColor colour = QColor(rgb);
            emit(colourSelected(colour));
        }
    }

}

void PaletteView::resizeEvent(QResizeEvent* event){
    QWidget::resizeEvent(event);

   //  qDebug() << "resizeEvent" << event->size();

    /*
    int w = mLabel->width() - 100;
    int h = mLabel->height();
    mLabel->setPixmap(mLabel->pixmap()->scaled(w, h, Qt::KeepAspectRatioByExpanding));
    */
}

