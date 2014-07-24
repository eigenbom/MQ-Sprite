#ifndef PARTWIDGET_H
#define PARTWIDGET_H

#include "projectmodel.h"

#include <QMdiSubWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImage>
#include <QVector>
#include <QGraphicsPixmapItem>
#include <QGraphicsSimpleTextItem>

enum DrawToolType {kDrawToolPaint, kDrawToolEraser, kDrawToolPickColour, kDrawToolFill, kDrawToolStamp, kDrawToolCopy};

/////////////////////////////////////////////
// PartView
/////////////////////////////////////////////

// See ImageViewer example: http://qt-project.org/doc/qt-5.0/qtwidgets/widgets-imageviewer-imageviewer-cpp.html
// And Scribble: http://qt-project.org/doc/qt-5.0/qtwidgets/widgets-scribble.html

class PartWidget;
class PartView: public QGraphicsView {
    Q_OBJECT
    friend class PartWidget;
public:
    PartView(PartWidget* parent, QGraphicsScene* scene);
protected:
    // Forwards these to parent
    void mousePressEvent(QMouseEvent *mouseEvent);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mouseReleaseEvent(QMouseEvent *mouseEvent);
    void wheelEvent(QWheelEvent *event);
    void drawBackground(QPainter *painter, const QRectF &rect);
    void keyPressEvent(QKeyEvent *event);
    void scrollContentsBy(int, int);

protected:
    PartWidget* pw;
};

////////////////////////////////////////////////
// PartWidget
////////////////////////////////////////////////

class PartWidget : public QMdiSubWindow
{
    Q_OBJECT
    friend class PartView;
public:
    explicit PartWidget(AssetRef partRef, QWidget *parent = 0);

    // Project Model updates
    void setMode(const QString& mode);

    void partNameChanged(const QString& newPartName);    
    void partFrameUpdated(AssetRef part, const QString& mode, int frame);
    void partFramesUpdated(AssetRef part, const QString& mode);
    void partNumPivotsUpdated(AssetRef part, const QString& mode);
    void partPropertiesChanged(AssetRef part);

    // updaters
    void updatePropertiesOverlays();
    void updateOverlay();
    void updatePartFrames();

    // setters
    void setDrawToolType(DrawToolType type);
    void setFrame(int f);
    void setPlaybackSpeedMultiplier(int index, float value);

    // query
    AssetRef partRef() const {return mPartRef;}
    QString partName() const {return mPartName;}
    QString modeName() const {return mModeName;}
    int zoom() const {return mZoom;}
    int penSize() const {return mPenSize;}
    QString properties() const {return mProperties;}
    QColor penColour() const {return mPenColour;}
    DrawToolType drawToolType() const {return mDrawToolType;}    
    bool isPlaying() const {return mIsPlaying;}
    int frame() const {return mFrameNumber;}
    int numFrames() const {return mPixmapItems.size();}
    int numPivots() const {return mNumPivots;}
    int playbackSpeedMultiplierIndex() const {return mPlaybackSpeedMultiplierIndex;}

protected:
    void closeEvent(QCloseEvent *event);

    void partViewMousePressEvent(QMouseEvent *mouseEvent);
    void partViewMouseMoveEvent(QMouseEvent *mouseEvent);
    void partViewMouseReleaseEvent(QMouseEvent *mouseEvent);
    void partViewWheelEvent(QWheelEvent *event);
    void partViewKeyPressEvent(QKeyEvent *event);

    void drawLineTo(const QPoint &endPoint);
    void eraseLineTo(const QPoint &endPoint);

    void enterEvent(QEvent *);

signals:
    void penChanged();
    void zoomChanged();
    void closed(PartWidget*);

    void frameChanged(int);
    void numPartFramesUpdated();
    void playActivated(bool);

    void selectNextMode();
    void selectPreviousMode();

public slots:
    void setZoom(int);
    void setPenSize(int);
    void setPenColour(QColor);
    void fitToWindow();
    void updateDropShadow();
    void updateOnionSkinning();
    void updatePivots();
    void setPosition(QPointF);

    void selectColourUnderPoint(QPointF);
    void showFrame(int f);

    void play(bool);
    void stop();

    void updateAnimation();

protected:
    AssetRef mPartRef;
    QString mPartName;
    QString mModeName;
    Part* mPart;
    PartView* mPartView;
    QGraphicsPixmapItem* mOverlayPixmapItem;

    float mZoom;
    QPointF mPosition;
    int mPenSize;
    QColor mPenColour;
    QColor mEraserColour;
    DrawToolType mDrawToolType;
    QTimer* mAnimationTimer;
    int mFrameNumber;
    double mSecondsPassedSinceLastFrame; // TODO: change to seconds since frame?
    bool mIsPlaying;

    QGraphicsRectItem* mBoundsItem;
    QVector<QGraphicsPixmapItem*> mPixmapItems;
    QVector<QGraphicsSimpleTextItem*> mAnchorItems;
    QVector<QGraphicsSimpleTextItem*> mPivotItems[MAX_PIVOTS];    
    QVector<QGraphicsRectItem*> mRectItems;
    QVector<QPoint> mAnchors;
    QVector<QPoint> mPivots[MAX_PIVOTS];
    int mNumPivots;
    QString mProperties;
    int mFPS;
    double mSPF;
    int mPlaybackSpeedMultiplierIndex;
    float mPlaybackSpeedMultiplier;

    QPoint mLastPoint;
    QPoint mTopLeftPoint;
    QPoint mMousePos;
    QPointF mLastCanvasPosition;
    bool mScribbling;
    bool mMovingCanvas;
    QImage* mOverlayImage; // TODO: resize this when change mode
    float mOnionSkinningTransparency;
    bool mOnionSkinningEnabled;
    bool mOnionSkinningEnabledDuringPlayback;

    QColor mPivotColour;
    bool mPivotsEnabled;
    bool mPivotsEnabledDuringPlayback;

    QPixmap mClipboardImage;
    QGraphicsPixmapItem* mClipboardItem;

    QGraphicsRectItem* mCopyRectItem;
};

#endif // PARTWIDGET_H
