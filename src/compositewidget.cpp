#include "compositewidget.h"
#include "commands.h"
#include "mainwindow.h"

#include <cmath>
#include <QUndoStack>
#include <QCloseEvent>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QDebug>
#include <QRgb>
#include <QMdiSubWindow>
#include <QGraphicsDropShadowEffect>
#include <QSettings>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

CompositeWidget::CompositeWidget(const QString& compName, QWidget *parent) :
    QMdiSubWindow(parent),
    mCompName(compName),
    mComp(NULL),
    mCompView(NULL),
    mZoom(4),
    mPosition(0,0),
    mAnimationTimer(NULL),
    mIsPlaying(false),
    mMovingCanvas(false),
    mSecondsPassedSinceLastFrame(0),
    mPlaybackSpeedMultiplierIndex(-1),
    mPlaybackSpeedMultiplier(1),
    mBoundsRect(),
    mProperties()
{
    // Customise window
    setMinimumSize(64,64);

    // Setup view
    mCompView = new CompositeView(this, new QGraphicsScene());
    mCompView->setTransform(QTransform::fromScale(mZoom,mZoom));
    setWidget(mCompView);
    updateCompFrames();
}

void CompositeWidget::updateCompFrames(){
    //////////////////////
    // Load all frames of all modes of all parts
    //////////////////////

    setWindowTitle(mCompName);

    // Delete all existing graphics items
    foreach(const ChildDriver& cd, mChildrenMap.values()){
        if (!cd.valid) continue;
        foreach(const ChildDriver::Mode& mode, cd.modes.values()){
            foreach(QGraphicsPixmapItem* pi, mode.pixmapItems){
                mCompView->scene()->removeItem(pi);
                delete pi;
            }
        }
    }
    mChildrenMap.clear();
    mChildren.clear();

    foreach(QGraphicsRectItem* pi, mRectItems){
        mCompView->scene()->removeItem(pi);
        delete pi;
    }
    mRectItems.clear();

    mCompView->scene()->clear();

    // TODO: Add origin?
    // mCompView->scene()->addRect(-.25,-.25,.5,.5,QPen(Qt::NoPen),QBrush(QColor(0,0,0,50)))->setZValue(1);

    // Load all children
    Composite* comp = PM()->getComposite(mCompName);
    QRectF fullBounds;
    if (comp != nullptr){
        mChildren = comp->children;
        mRoot = comp->root;
        mProperties = comp->properties;

        foreach(const QString& childName, mChildren){
            ChildDriver cd;
            const Composite::Child& child = comp->childrenMap.value(childName);

            //////////////////////////////////
            // Load part
            //////////////////////////////////
            cd.part = child.part;
            cd.children = child.children;
            cd.index = child.index;
            cd.parent = child.parent;
            cd.parentPivot = child.parentPivot;
            cd.z = child.z;
            cd.visible = true;

            cd.mode = QString();
            cd.loop = true;
            cd.accumulator = 0;
            cd.frame = 0;
            cd.updated = false;
            cd.valid = false;

            if (PM()->hasPart(cd.part)){
                Part* part = PM()->getPart(cd.part);
                // qDebug() << "partName: " << cd.part;

                // Load all modes...
                foreach(QString modeName, part->modes.keys()){
                    cd.valid = true; // must have one mode to be valid

                    // qDebug() << "modeName: " << modeName;
                    if (cd.mode.isNull() || cd.mode.isEmpty()) cd.mode = modeName;
                    const Part::Mode& m = part->modes.value(modeName);

                    ChildDriver::Mode mode;
                    mode.numFrames = m.numFrames;
                    mode.numPivots = m.numPivots;
                    mode.fps = m.framesPerSecond;
                    mode.spf = 1./mode.fps;
                    mode.width = m.width;
                    mode.height = m.height;

                    bool showBounds = false;
                    if (!showBounds){
                        mode.boundsItem = NULL;
                    }
                    else {
                        mode.boundsItem = mCompView->scene()->addRect(-0.5,-0.5,mode.width+1,mode.height+1, QPen(QColor(0,0,0), 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin), QBrush(QColor(0,0,0,0)));
                        mode.boundsItem->setZValue(-1);
                        mode.boundsItem->hide();
                        if (cd.mode==modeName){
                            mode.boundsItem->show();
                        }
                    }

                    mode.anchors = QVector<QPoint>::fromList(m.anchor);
                    for(int p=0;p<mode.numPivots;p++){
                        mode.pivots[p] = QVector<QPoint>::fromList(m.pivots[p]);
                    }

                    for(int i=0;i<m.numFrames;i++){
                        QImage* img = m.images.at(i);
                        // qDebug() << "img: " << img;
                        if (img!=NULL){
                            QGraphicsPixmapItem* pi = mCompView->scene()->addPixmap(QPixmap::fromImage(*img));
                            QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
                            pi->setGraphicsEffect(effect);
                            pi->setZValue(cd.z);
                            mode.pixmapItems.push_back(pi);

                            if (fullBounds.isNull()) fullBounds = pi->boundingRect();
                            else fullBounds = fullBounds.united(pi->boundingRect());
                        }
                        else {
                            QImage img(mode.width, mode.height, QImage::Format_ARGB32);
                            img.fill(0xFFFF00FF);
                            QGraphicsPixmapItem* pi = mCompView->scene()->addPixmap(QPixmap::fromImage(img));
                            pi->setZValue(cd.z);
                            mode.pixmapItems.push_back(pi);

                            if (fullBounds.isNull()) fullBounds = pi->boundingRect();
                            else fullBounds = fullBounds.united(pi->boundingRect());
                        }
                    }

                    cd.modes.insert(modeName,mode);
                }
            }
            else {
                // do nothing
            }

            mChildrenMap.insert(childName, cd);
        }
    }
    else {
        mCompView->scene()->addSimpleText(mCompName);
    }

    // Center
    // qDebug() << fullBounds;
    // setPosition(fullBounds.topLeft()/mZoom);
    setZoom(mZoom);
    updateDropShadow();
    updateFrame();
    updatePropertiesOverlays();
}

void CompositeWidget::updateCompFramesMinorChanges(){
    //////////////////////
    // Update all frames of all modes of all parts, assume no new children have been added etc
    //////////////////////

    foreach(QGraphicsRectItem* pi, mRectItems){
        mCompView->scene()->removeItem(pi);
        delete pi;
    }
    mRectItems.clear();

    // Update all children
    Composite* comp = PM()->getComposite(mCompName);
    QRectF fullBounds;
    if (comp){
        mRoot = comp->root;
        mProperties = comp->properties;

        foreach(const QString& childName, mChildren){
            ChildDriver& cd = mChildrenMap[childName];
            const Composite::Child& child = comp->childrenMap.value(childName);

            //////////////////////////////////
            // Load part
            //////////////////////////////////
            bool changePart = false;
            if (cd.part!=child.part){
                cd.part = child.part;
                changePart = true;

                // remove old modes..
                QMutableMapIterator<QString,ChildDriver::Mode> it(cd.modes);
                while (it.hasNext()){
                    it.next();
                    ChildDriver::Mode& mode = it.value();
                    if (mode.boundsItem) mCompView->scene()->removeItem(mode.boundsItem);
                    for(int i=0;i<mode.numFrames;i++){
                        mCompView->scene()->removeItem(mode.pixmapItems.at(i));
                    }
                }
                cd.modes.clear();
            }

            cd.children = child.children;
            cd.index = child.index;
            cd.parent = child.parent;
            cd.parentPivot = child.parentPivot;
            cd.z = child.z;

            cd.valid = false;

            if (PM()->hasPart(cd.part)){
                Part* part = PM()->getPart(cd.part);
                // qDebug() << "partName: " << cd.part;

                // Load all modes...
                foreach(QString modeName, part->modes.keys()){
                    cd.valid = true; // must have one mode to be valid

                    // qDebug() << "modeName: " << modeName;
                    if (cd.mode.isNull() || cd.mode.isEmpty()) cd.mode = modeName;
                    const Part::Mode& m = part->modes.value(modeName);

                    bool hasMode = cd.modes.count(modeName)>0;
                    if (!hasMode){
                        cd.modes[modeName] = ChildDriver::Mode();
                    }

                    ChildDriver::Mode& mode = cd.modes[modeName];
                    mode.numFrames = m.numFrames;
                    mode.numPivots = m.numPivots;
                    mode.fps = m.framesPerSecond;
                    mode.spf = 1./mode.fps;
                    mode.width = m.width;
                    mode.height = m.height;

                    bool showBounds = false;
                    if (!showBounds){
                        mode.boundsItem = NULL;
                    }
                    else {
                        if (mode.boundsItem) mCompView->scene()->removeItem(mode.boundsItem);
                        mode.boundsItem = mCompView->scene()->addRect(-0.5,-0.5,mode.width+1,mode.height+1, QPen(QColor(0,0,0), 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin), QBrush(QColor(0,0,0,0)));
                        mode.boundsItem->setZValue(-1);
                        mode.boundsItem->hide();
                        if (cd.mode==modeName){
                            mode.boundsItem->show();
                        }
                    }

                    mode.anchors = QVector<QPoint>::fromList(m.anchor);
                    for(int p=0;p<mode.numPivots;p++){
                        mode.pivots[p] = QVector<QPoint>::fromList(m.pivots[p]);
                    }

                    for(int i=0;i<m.numFrames;i++){
                        QImage* img = m.images.at(i);
                        // qDebug() << "img: " << img;
                        if (img!=NULL){
                            if (hasMode) mCompView->scene()->removeItem(mode.pixmapItems.at(i));
                            QGraphicsPixmapItem* pi = mCompView->scene()->addPixmap(QPixmap::fromImage(*img));
                            QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
                            pi->setGraphicsEffect(effect);
                            pi->setZValue(cd.z);
                            if (hasMode) mode.pixmapItems.replace(i, pi);
                            else mode.pixmapItems.push_back(pi);


                            if (fullBounds.isNull()) fullBounds = pi->boundingRect();
                            else fullBounds = fullBounds.united(pi->boundingRect());
                        }
                        else {
                            if (hasMode) mCompView->scene()->removeItem(mode.pixmapItems.at(i));

                            QImage img(mode.width, mode.height, QImage::Format_ARGB32);
                            img.fill(0xFFFF00FF);
                            QGraphicsPixmapItem* pi = mCompView->scene()->addPixmap(QPixmap::fromImage(img));
                            pi->setZValue(cd.z);
                            if (hasMode) mode.pixmapItems.replace(i, pi);
                            else mode.pixmapItems.push_back(pi);

                            if (fullBounds.isNull()) fullBounds = pi->boundingRect();
                            else fullBounds = fullBounds.united(pi->boundingRect());
                        }
                    }
                }
            }
            else {
                // do nothing
            }
        }
    }
    else {
        mCompView->scene()->addSimpleText(mCompName);
    }

    // Center
    // qDebug() << fullBounds;
    // setPosition(fullBounds.topLeft()/mZoom);
    setZoom(mZoom);
    updateDropShadow();
    updateFrame();
    updatePropertiesOverlays();
}


void CompositeWidget::updateFrame(){
    // First, recursivley compute the absolute positions of things...
    QMutableMapIterator<QString,ChildDriver> it(mChildrenMap);
    while (it.hasNext()){        
        it.next();
        ChildDriver& cd = it.value();
        cd.updated = false;
        cd.parentPivotOffset = QPointF(0,0);
    }

    it.toFront();
    while (it.hasNext()){
        it.next();
        ChildDriver& cd = it.value();

        if (cd.parent==-1 && cd.valid){
            if (cd.modes.contains(cd.mode)){
                QPoint anchor = cd.modes[cd.mode].anchors[cd.frame];
                updateChildRecursively(cd.index, -anchor);
            }
            else {
                updateChildRecursively(cd.index, QPointF(0,0));
            }
        }
    }

    //  update the positions and visibility of all the pixmaps and bounds..

    mBoundsRect = QRectF();

    it = QMutableMapIterator<QString,ChildDriver>(mChildrenMap);
    while (it.hasNext()){
        it.next();
        ChildDriver& cd = it.value();
        if (!cd.valid) continue;

        QMutableMapIterator<QString,ChildDriver::Mode> mit(cd.modes);
        while (mit.hasNext()){
            mit.next();
            ChildDriver::Mode& mode = mit.value();
            bool correctMode = (mit.key()==cd.mode);
            if (mode.boundsItem){
                if (correctMode) mode.boundsItem->show();
                else mode.boundsItem->hide();
            }

            for(int f=0;f<mode.numFrames;f++){                
                mode.pixmapItems[f]->hide();
                if (mode.pixmapItems[f]->graphicsEffect())
                    mode.pixmapItems[f]->graphicsEffect()->setEnabled(false);
                if (mode.boundsItem) mode.boundsItem->setPos(cd.parentPivotOffset);                
                if (cd.visible && f==cd.frame && correctMode){
                    mode.pixmapItems[f]->setPos(cd.parentPivotOffset);
                    mode.pixmapItems[f]->show();

                    if (mode.pixmapItems[f]->graphicsEffect())
                        mode.pixmapItems[f]->graphicsEffect()->setEnabled(true);

                    /*
                    if (mBoundsRect.isNull())
                        mBoundsRect = mode.pixmapItems[f]->boundingRect().translated(cd.parentPivotOffset.x(),cd.parentPivotOffset.y());
                    else
                        mBoundsRect.united(mode.pixmapItems[f]->boundingRect()).translated(cd.parentPivotOffset.x(),cd.parentPivotOffset.y());
                    */

                    if (mBoundsRect.isNull())
                        mBoundsRect = mode.pixmapItems[f]->boundingRect();
                    else
                        mBoundsRect.united(mode.pixmapItems[f]->boundingRect());
                }
            }
        }
    }
}

void CompositeWidget::updateChildRecursively(int index, const QPointF& parentPivotPos){
    if (index==-1) return;
    const QString& name = mChildren.at(index);
    if (mChildrenMap.contains(name)){
        ChildDriver& cd = mChildrenMap[name];

        if (!cd.valid) return;
        if (cd.updated) return; // Whoops, already updated, mustve been something wrong in the tree

        cd.parentPivotOffset = parentPivotPos;
        cd.updated = true;
        if (cd.modes.contains(cd.mode)){
            ChildDriver::Mode& mode = cd.modes[cd.mode];
            foreach(int childIndex, cd.children){
                const QString& childName = mChildren.at(childIndex);
                ChildDriver& child = mChildrenMap[childName];
                if (child.modes.contains(child.mode) && child.valid){
                    ChildDriver::Mode& chmode = child.modes[child.mode];
                    QPointF pp = parentPivotPos;
                    if (child.parentPivot>=0 && child.parentPivot<mode.numPivots){
                        pp += (mode.pivots[child.parentPivot][cd.frame] - chmode.anchors[child.frame]);
                    }
                    updateChildRecursively(childIndex, pp);
                }
            }
        }
    }
}

void CompositeWidget::updatePropertiesOverlays(){
    // Parse the properties
    // trying to extra any info that we can draw into the scene
    foreach(QGraphicsRectItem* pi, mRectItems){
        mCompView->scene()->removeItem(pi);
        delete pi;
    }
    mRectItems.clear();

    QJsonParseError error;
    QJsonDocument propDoc = QJsonDocument::fromJson(QByteArray(mProperties.toStdString().c_str()), &error);
    // QJsonDocument prefsDoc = QJsonDocument::fromJson(QByteArray(mProperties.buffer, mProperties.length), &error);

    if (error.error!=QJsonParseError::NoError || propDoc.isNull() || propDoc.isEmpty() || !propDoc.isObject()){
        // couldn't parse.. (or not object)
    }
    else {
        static const int NUM_COLOURS = 4;
        static const QColor COLOURS[NUM_COLOURS] = {QColor(255,0,255), QColor(150,100,255), QColor(255,100,150), QColor(200,150,200)};

        int index = 0;
        QJsonObject propObj = propDoc.object();
        for(QJsonObject::iterator it = propObj.begin(); it!=propObj.end(); it++){
            QString key = it.key();

            if (key.endsWith("_rect") && it.value().isArray()){
                // draw a rectangle over the sprite

                const QJsonArray& array = it.value().toArray();
                 if (array.size()==4){
                    double x = array.at(0).toDouble(0);
                    double y = array.at(1).toDouble(0);
                    double w = array.at(2).toDouble(1);
                    double h = array.at(3).toDouble(1);

                    QPen pen = QPen(COLOURS[index%NUM_COLOURS], 0.1);
                    QBrush brush = Qt::NoBrush;
                    QGraphicsRectItem* item = mCompView->scene()->addRect(x,y,w,h,pen,brush);
                    // qDebug() << "Added rect item: " << x << "," << y << "," << w << "," << h;
                    item->setZValue(10);
                    mRectItems.append(item);

                    index++;
                 }
            }
        }
    }
}

void CompositeWidget::partNameChanged(const QString& oldPartName, const QString& newPartName){
    // Check for any children with this name...
    QMutableMapIterator<QString,ChildDriver> it(mChildrenMap);
    while (it.hasNext()){
        it.next();
        ChildDriver& cd = it.value();
        if (cd.part==oldPartName){
            // qDebug() << cd.part << "->" << newPartName;
            cd.part = newPartName;
        }
    }
}

void CompositeWidget::partFrameUpdated(const QString& part, const QString& /*mode*/, int /*frame*/){
    QMutableMapIterator<QString,ChildDriver> it(mChildrenMap);
    bool bingo = false;
    while (it.hasNext()){
        it.next();
        ChildDriver& cd = it.value();
        if (cd.part==part){
            bingo = true;
            break;
        }
    }

    if (bingo) updateCompFramesMinorChanges();
}

void CompositeWidget::partFramesUpdated(const QString& part, const QString&/* mode*/){
    QMutableMapIterator<QString,ChildDriver> it(mChildrenMap);
    bool bingo = false;
    while (it.hasNext()){
        it.next();
        ChildDriver& cd = it.value();
        if (cd.part==part){
            bingo = true;
            break;
        }
    }
    if (bingo) updateCompFramesMinorChanges();
}

void CompositeWidget::partNumPivotsUpdated(const QString& part, const QString& /*mode*/){
    QMutableMapIterator<QString,ChildDriver> it(mChildrenMap);
    bool bingo = false;
    while (it.hasNext()){
        it.next();
        ChildDriver& cd = it.value();
        if (cd.part==part){
            bingo = true;
            break;
        }
    }
    if (bingo) updateCompFramesMinorChanges();
}

void CompositeWidget::setZoom(int z){
    mZoom = z;
    mCompView->setTransform(QTransform::fromScale(mZoom,mZoom));
    mCompView->setSceneRect(mCompView->scene()->sceneRect().translated(mPosition.x()/mZoom, mPosition.y()/mZoom));
    updateDropShadow();
    mCompView->update();
}

void CompositeWidget::compNameChanged(const QString& name){
    mCompName = name;
    setWindowTitle(mCompName);
}

void CompositeWidget::compPropertiesChanged(const QString& name){
    if (name==mCompName){
        // update ..
        Composite* comp = PM()->getComposite(mCompName);
        mProperties = comp->properties;
        updatePropertiesOverlays();
        mCompView->update();
    }
}

void CompositeWidget::setChildMode(const QString& child, const QString& mode){
    bool wasPlaying = mIsPlaying;
    if (mIsPlaying) play(false);

    QMap<QString,ChildDriver>::iterator it = mChildrenMap.find(child);
    if (it!=mChildrenMap.end()){
        it->mode = mode;
    }

    if (wasPlaying) play(true);
    else updateFrame();
}

void CompositeWidget::setChildLoop(const QString& child, bool loop){
    bool wasPlaying = mIsPlaying;
    if (mIsPlaying) play(false);

    QMap<QString,ChildDriver>::iterator it = mChildrenMap.find(child);
    if (it!=mChildrenMap.end()){
        it->loop = loop;
    }

    if (wasPlaying) play(true);
}

void CompositeWidget::setChildVisible(const QString& child, bool visible){
    bool wasPlaying = mIsPlaying;
    if (mIsPlaying) play(false);

    QMap<QString,ChildDriver>::iterator it = mChildrenMap.find(child);
    if (it!=mChildrenMap.end()){
        it->visible = visible;
    }

    if (wasPlaying) play(true);
    else updateFrame();
}

void CompositeWidget::setPlaybackSpeedMultiplier(int index, float value){
    mPlaybackSpeedMultiplierIndex = index;
    mPlaybackSpeedMultiplier = value;
}

QString CompositeWidget::modeForCurrentSet(const QString& child) const {
    if (!mChildrenMap.contains(child)) return QString();
    // else ..
    const ChildDriver& driver = mChildrenMap.value(child);
    return driver.mode;
}

bool CompositeWidget::loopForCurrentSet(const QString& child) const {
    if (!mChildrenMap.contains(child)) return false;
    // else ..
    const ChildDriver& driver = mChildrenMap.value(child);
    return driver.loop;
}

bool CompositeWidget::visibleForCurrentSet(const QString& child) const{
    if (!mChildrenMap.contains(child)) return true;
    // else ..
    const ChildDriver& driver = mChildrenMap.value(child);
    return driver.visible;
}

void CompositeWidget::setPosition(QPointF pos){
    // qDebug() << "setPosition" << mPosition;

    mPosition = pos;
    mCompView->setTransform(QTransform::fromScale(mZoom,mZoom));
    mCompView->setSceneRect(mCompView->scene()->sceneRect().translated(pos.x()/mZoom,pos.y()/mZoom));
    mCompView->update();
}

void CompositeWidget::closeEvent(QCloseEvent *event)
{
    emit(closed(this));
    event->accept();
}

void CompositeWidget::resizeEvent(QResizeEvent *resizeEvent){
    // Zoom..
    // use graphicsview fit to window
    /*
    if (mCompView){
        // qDebug() << "resizeEvent" << mBoundsRect;
        QRectF b = mBoundsRect;
        b.setWidth(b.width()*2);
        b.setHeight(b.height()*2);

        mCompView->fitInView(b, Qt::KeepAspectRatio); // Qt::KeepAspectRatioByExpanding);

        QTransform transform = mCompView->transform();
        QPoint scale = transform.map(QPoint(1,1));
        mZoom = floor(scale.x());
        mPosition = QPoint(0,0);
        setZoom(mZoom);
        zoomChanged();
    }
    */

    QMdiSubWindow::resizeEvent(resizeEvent);
}

void CompositeWidget::compViewMousePressEvent(QMouseEvent *event){
    mLastPoint = event->pos();
    // bool left = event->button()==Qt::LeftButton;
    // bool right = event->button()==Qt::RightButton;
    bool middle = event->button()==Qt::MiddleButton;

    if (middle){
        mLastCanvasPosition = mPosition;
        mMovingCanvas = true;
    }
}

void CompositeWidget::compViewMouseMoveEvent(QMouseEvent *event){
    mMousePos = event->pos();

    // bool left = event->buttons()&Qt::LeftButton;
    // bool right = event->buttons()&Qt::RightButton;
    bool middle = event->buttons()&Qt::MiddleButton;

    if (middle && mMovingCanvas){
        QPoint pos = event->pos();
        QPoint diff = mLastPoint - pos;
        setPosition(mLastCanvasPosition + QPointF(diff));
    }
}

void CompositeWidget::compViewMouseReleaseEvent(QMouseEvent *event){
    if (event->button()==Qt::MiddleButton && mMovingCanvas){
        mMovingCanvas = false;
    }
}

void CompositeWidget::compViewWheelEvent(QWheelEvent *event){
    if (event->modifiers()&Qt::ControlModifier){
        QPoint pt = event->angleDelta();
        if (pt.y()>0){
            // zoom in
            if (zoom()<48){
                // zoom in proportional to current zoom..
                int nz = std::min(48,zoom()*2);
                setZoom(nz);
                emit(zoomChanged());
            }
        }
        else if (pt.y()<0) {
            // zoom out
            if (zoom()>1){
                int nz = std::max(1, zoom()/2);
                setZoom(nz);
                emit(zoomChanged());
            }
        }
    }
}

void CompositeWidget::compViewKeyPressEvent(QKeyEvent *event){
    // QPointF coordf = mCompView->mapToScene(mMousePos.x(),mMousePos.y());
    // QPoint coord(floor(coordf.x()), floor(coordf.y()));

    switch (event->key()){
        case Qt::Key_Space: {
            // Toggle play/pause
            if (mIsPlaying) play(false);
            else play(true);

            emit (playActivated(mIsPlaying));
            break;
        }
    }
}

static float S_PER_UPDATE = 1.f/60;
void CompositeWidget::play(bool play){
    if (!play){
        mIsPlaying = false;
        if (mAnimationTimer){
            mAnimationTimer->stop();
        }

        // Reset everything to start state..
        QMutableMapIterator<QString,ChildDriver> it(mChildrenMap);
        while (it.hasNext()){
            it.next();
            ChildDriver& cd = it.value();
            cd.accumulator = 0;
            cd.frame = 0;
        }
        updateFrame();
    }
    else {
        if (mAnimationTimer==NULL){
            mAnimationTimer=new QTimer(this);
            connect(mAnimationTimer, SIGNAL(timeout()), this, SLOT(updateAnimation()));
        }

        QMutableMapIterator<QString,ChildDriver> it(mChildrenMap);
        while (it.hasNext()){
            it.next();
            ChildDriver& cd = it.value();
            cd.accumulator = 0;
            cd.frame = 0;
        }
        mAnimationTimer->start(S_PER_UPDATE*1000);
        mIsPlaying = true;
    }
}

void CompositeWidget::updateAnimation(){
    bool hasChildFrameChanged = false;
    float ds = mPlaybackSpeedMultiplier*S_PER_UPDATE;
    QMutableMapIterator<QString,ChildDriver> it(mChildrenMap);
    while (it.hasNext()){
        it.next();
        ChildDriver& cd = it.value();
        cd.accumulator += ds;
        if (!cd.valid) continue;

        const ChildDriver::Mode& mode = cd.modes.value(cd.mode);
        bool updated = false;
        while (cd.accumulator > mode.spf){
            updated = true;
            hasChildFrameChanged = true;
            cd.accumulator -= mode.spf;
            if (cd.loop)
                cd.frame = (cd.frame+1)%mode.numFrames;
            else
                cd.frame = std::min<int>(cd.frame+1, mode.numFrames-1);
        }

        if (updated){
            // just do a brute force update below for now ..
        }
    }

    if (hasChildFrameChanged){
        updateFrame();
    }
}

void CompositeWidget::updateDropShadow(){
    QSettings settings;
    float opacity = settings.value("drop_shadow_opacity", QVariant(100.f/255)).toFloat();
    float blur = settings.value("drop_shadow_blur", QVariant(0.5f)).toFloat();
    float dx = settings.value("drop_shadow_offset_x", QVariant(0.1f)).toFloat();
    float dy = settings.value("drop_shadow_offset_y", QVariant(0.1f)).toFloat();
    QColor dropShadowColour = QColor(settings.value("drop_shadow_colour", QColor(0,0,0).rgba()).toUInt());

    if (mCompView){
        QMutableMapIterator<QString,ChildDriver> it(mChildrenMap);
        while (it.hasNext()){
            it.next();
            ChildDriver& cd = it.value();
            if (!cd.valid) continue;

            QMutableMapIterator<QString,ChildDriver::Mode> mit(cd.modes);
            while (mit.hasNext()){
                mit.next();
                ChildDriver::Mode& m = mit.value();
                foreach(QGraphicsPixmapItem* it, m.pixmapItems){
                    QGraphicsDropShadowEffect* effect = (QGraphicsDropShadowEffect*) it->graphicsEffect();
                    effect->setOffset(dx*2*mZoom/4.,dy*2*mZoom/2.);
                    effect->setColor(QColor(dropShadowColour.red(),dropShadowColour.green(),dropShadowColour.blue(),opacity*255));
                    effect->setBlurRadius(blur*2*mZoom*2.);
                }
            }
        }
    }
}

////////////////////////////////////////////////
// Composite View
////////////////////////////////////////////////

CompositeView::CompositeView(CompositeWidget *parent, QGraphicsScene *scene)
    :QGraphicsView(scene, parent),
      cw(parent)
{
    setMouseTracking(true);
    setCursor(QCursor(QPixmap::fromImage(QImage(":/icon/icons/tool_none.png")),8,8));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void CompositeView::mousePressEvent(QMouseEvent *event){
    cw->compViewMousePressEvent(event);
}

void CompositeView::mouseMoveEvent(QMouseEvent *event){
    cw->compViewMouseMoveEvent(event);
}

void CompositeView::mouseReleaseEvent(QMouseEvent *event){
    cw->compViewMouseReleaseEvent(event);
}

void CompositeView::wheelEvent(QWheelEvent *event){
    cw->compViewWheelEvent(event);
}

void CompositeView::keyPressEvent(QKeyEvent *event){
    cw->compViewKeyPressEvent(event);
}

void CompositeView::drawBackground(QPainter *painter, const QRectF &rect)
{
    QSettings settings;
    // QColor backgroundColour = QColor(settings.value("background_colour", QColor(111,198,143).rgba()).toUInt());
    QColor backgroundColour = QColor(settings.value("background_colour", QColor(255,255,255).rgba()).toUInt());
    painter->fillRect(rect, backgroundColour);

    // Calculate good colour for bounds rect
    QColor boundsColour = backgroundColour;
    int h = backgroundColour.hue();
    int s = backgroundColour.saturation();
    int val = backgroundColour.value();

    if (val < 50){
        val += 30;
    }
    else if (val < 255/2){
        val *= 1.2;
    }
    else if (val < (255/2 - 50)){
        val /= 1.2;
    }
    else {
        val -= 30;
    }
    boundsColour.setHsv(h,s,val);

    QMutableMapIterator<QString,CompositeWidget::ChildDriver> it(cw->mChildrenMap);
    while (it.hasNext()){
        it.next();
        CompositeWidget::ChildDriver& cd = it.value();
        if (cd.valid && cd.modes.contains(cd.mode)){
            CompositeWidget::ChildDriver::Mode& m = cd.modes[cd.mode];
            if (m.boundsItem){
                // qDebug() << "Setting boundsItem colour to: " << boundsColour;
                QPen pen = m.boundsItem->pen();
                pen.setColor(boundsColour);
                m.boundsItem->setPen(pen);
            }
        }
    }
}

