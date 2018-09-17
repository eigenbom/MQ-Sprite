#include "partwidget.h"

#include "commands.h"
#include "mainwindow.h"
#include "spritezoomwidget.h"

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
#include <QSlider>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QBuffer>
#include <QQueue>
#include <QToolButton>

PartWidget::PartWidget(AssetRef ref, QWidget *parent) :
    // Qt::FramelessWindowHint
    // QMdiSubWindow(parent, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint),
	QMdiSubWindow(parent, Qt::SubWindow),
    mPartRef(ref),
    mPartName(),
    mModeName("icon"),
    mPart(nullptr),
    mPartView(nullptr),
    mOverlayPixmapItem(nullptr),
    mZoom(4),
    mViewportCenter(0,0),
    mPenSize(1),
    mPenColour(QColor(0,0,0)),
    mDrawToolType(kDrawToolPaint),
    mAnimationTimer(nullptr),
    mFrameNumber(0),
    mSecondsPassedSinceLastFrame(0),
    mIsPlaying(false),
    mBoundsItem(nullptr),
    mScribbling(false),
    mMovingCanvas(false),
    mPlaybackSpeedMultiplierIndex(-1),
    mPlaybackSpeedMultiplier(1),
    mOverlayImage(nullptr),
    mClipboardItem(nullptr),
    mCopyRectItem(nullptr)
{
    setMinimumSize(256, 256);
	setWindowIcon(QIcon { ":/icon/icons/gentleface/picture_icon&16.png" });

    mPartView = new PartView(this, new QGraphicsScene());
    mPartView->setTransform(QTransform::fromScale(mZoom,mZoom));
	mViewportCenter = QPointF { 0, 0 };
	mPartView->centerOn(mViewportCenter);
				
	auto* frame = new QFrame();			
	auto* vbox = new QVBoxLayout();
	vbox->setMargin(0);
	vbox->setSpacing(0);
	vbox->addWidget(mPartView);
	auto* spriteZoomWidget = new SpriteZoomWidget(frame);
	vbox->addWidget(spriteZoomWidget);
	frame->setLayout(vbox);
	setWidget(frame);
	connect(spriteZoomWidget->findChild<QSlider*>("hSliderZoom"), SIGNAL(valueChanged(int)), this, SLOT(setZoom(int)));
	connect(spriteZoomWidget->findChild<QToolButton*>("toolButtonFitToWindow"), SIGNAL(clicked()), this, SLOT(fitToWindow()));
	connect(this, &PartWidget::zoomChanged, [this, spriteZoomWidget]() {
		spriteZoomWidget->findChild<QSlider*>("hSliderZoom")->setValue(this->zoom());
	});

    updateBackgroundBrushes();
    updatePartFrames();
}

void PartWidget::updatePartFrames(){
    if (!mPixmapItems.isEmpty()){
        foreach(QGraphicsPixmapItem* pi, mPixmapItems){
            mPartView->scene()->removeItem(pi);
            delete pi;
        }
        mPixmapItems.clear();
    }

    if (mOverlayPixmapItem!=nullptr){
        mPartView->scene()->removeItem(mOverlayPixmapItem);
        delete mOverlayPixmapItem;
        mOverlayPixmapItem = nullptr;
    }

    // get rid of any remaining text etc
    mAnchorItems.clear();
    mAnchors.clear();
    for(int i=0;i<MAX_PIVOTS;i++){
        mPivotItems[i].clear();
        mPivots[i].clear();
    }

    for(auto* pi: mPropertyItems){
        mPartView->scene()->removeItem(pi);
        delete pi;
    }
	mPropertyItems.clear();

    if (mClipboardItem!=nullptr){
        mPartView->scene()->removeItem(mClipboardItem);
        delete mClipboardItem;
        mClipboardItem = nullptr;
    }

    if (mCopyRectItem!=nullptr){
        mPartView->scene()->removeItem(mCopyRectItem);
        delete mCopyRectItem;
        mCopyRectItem = nullptr;
    }

    mPartView->scene()->clear();
    mBoundsItem = nullptr;
	bool triggerFitToWindow = false;

    if (PM()->hasPart(mPartRef)){
        mPart = PM()->getPart(mPartRef);
        mPartName = mPart->name;
        mProperties = "{" + mPart->properties + "}";

        if (mPart->modes.size() > 0){
            if (mModeName.isEmpty() || !mPart->modes.contains(mModeName)){
                mModeName = mPart->modes.begin().key();
            }
            const auto& m = mPart->modes.value(mModeName);
            mNumPivots = m.numPivots;            
            mFPS = m.framesPerSecond;
            mSPF = 1./mFPS;

            const int w = mPart->modes.value(mModeName).width;
			const int h = mPart->modes.value(mModeName).height;
            setWindowTitle(mPartName + tr(": ") + mModeName + tr(""));

			const int strokeAlpha = 255;
			mBoundsColour = mBackgroundColour.lightnessF() > 0.5 ? QColor(0, 0, 0, strokeAlpha) : QColor(255, 255, 255, strokeAlpha);
            const float boundsWidth = 0.05f;
            mBoundsItem = mPartView->scene()->addRect(-boundsWidth/2,-boundsWidth/2,w+boundsWidth,h+boundsWidth, QPen(mBoundsColour, boundsWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin), QBrush(QColor(0,0,0,0)));
			
            Q_ASSERT(m.numFrames>0);

            for(int i=0;i<m.numFrames;i++){
                auto pImg = m.frames.at(i);
                if (pImg){
                    QGraphicsPixmapItem* pi = mPartView->scene()->addPixmap(QPixmap::fromImage(*pImg));
                    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
                    pi->setGraphicsEffect(effect);
                    mPixmapItems.push_back(pi);
                }
                else {
                    mPartView->scene()->addSimpleText(tr("!"));
                }
            }

            if (mOverlayImage!=nullptr) delete mOverlayImage;
            mOverlayImage = new QImage(w,h,QImage::Format_ARGB32);
            mOverlayImage->fill(0x00FFFFFF);
            mOverlayPixmapItem = mPartView->scene()->addPixmap(QPixmap::fromImage(*mOverlayImage));
			
			QFont font("monospace");
            QPen pivotPen = QPen(mPropertiesColour, 0.1);

			const int anchorZ = 20;
            for(int i=0;i<m.numFrames;i++){
                {
                    QPoint ap = m.anchor.at(i);
                    mAnchors.push_back(ap);

					QList<QAbstractGraphicsShapeItem*> list;

					/*
					auto* it = mPartView->scene()->addRect(QRectF(0.25, 0.25, 0.5, 0.5));
					it->setPen(QPen(QColor("#000000"), 0.125 / 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
					it->setBrush(QBrush(QColor("#ffffff")));
					it->setPos(ap.x(), ap.y());
					mAnchorItems.push_back(it);
					*/

					// QPolygonF polygon({ QPointF(0.25, 0.5), QPointF(0.5,0.25), QPointF(0.75,0.5),  QPointF(0.5,0.75), });
					QPolygonF polygon({ QPointF(0.75,0.75),  QPointF(0.5,0.5), QPointF(0.25, 0.75) });
					
					auto* it = mPartView->scene()->addPolygon(polygon);
					it->setPen(Qt::NoPen); //  QPen(QColor(0, 0, 0, 128), 0.125 / 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
					it->setBrush(mPropertiesColour);
					it->setPos(ap.x(), ap.y());
					it->setZValue(anchorZ);
					list.push_back(it);
									   
					{
						auto* it = mPartView->scene()->addRect(QRectF(0.25, 0.75, 0.5, 0.5));
						it->setPen(Qt::NoPen);
						it->setBrush(mPropertiesColour);
						it->setPos(ap.x(), ap.y());
						it->setZValue(anchorZ);
						list.push_back(it);
					}

					{
						QGraphicsSimpleTextItem* label = mPartView->scene()->addSimpleText("A", font);
						label->setPen(Qt::NoPen);
						label->setBrush(QColor(255, 255, 255));
						label->setScale(0.05 / 2);
						label->setPos(ap.x() + 0.4, ap.y() + 0.68);
						label->setZValue(anchorZ + 2);
						list.push_back(label);
					}

					mAnchorItems.push_back(list);
                }

                for(int p=0;p<MAX_PIVOTS;p++){			
					QList<QAbstractGraphicsShapeItem*> list;

                    QPoint pp = m.pivots[p].at(i);
                    mPivots[p].push_back(pp);

					{
						QPolygonF polygon({ QPointF(0.25, 0.25), QPointF(0.5,0.5), QPointF(0.75,0.25) });
						auto* it = mPartView->scene()->addPolygon(polygon);
						it->setPen(Qt::NoPen);
						it->setBrush(mPropertiesColour);
						it->setPos(pp.x(), pp.y());
						it->setZValue(anchorZ);
						list.push_back(it);
					}

					{
						auto* it = mPartView->scene()->addRect(QRectF(0.25, -0.25, 0.5, 0.5));
						it->setPen(Qt::NoPen);
						it->setBrush(mPropertiesColour);
						it->setPos(pp.x(), pp.y());
						it->setZValue(anchorZ);
						list.push_back(it);
					}

					{
						QGraphicsSimpleTextItem* label = mPartView->scene()->addSimpleText(QString::number(p + 1), font);
						label->setPen(Qt::NoPen);
						label->setBrush(QColor(255,255,255));
						label->setScale(0.05 / 2);
						label->setPos(pp.x() + 0.4, pp.y() - 0.25);
						label->setZValue(anchorZ + 1);
						list.push_back(label);
					}

					mPivotItems[p].push_back(list);
                }
            }

            updateDropShadow();
            updateOnionSkinning();
            updatePivots();
            updatePropertiesOverlays();

            // Update draw tool type
            setDrawToolType(mDrawToolType);

			const bool firstTime = !mSeenMode.contains(mModeName);
			mSeenMode.insert(mModeName);
			if (firstTime) triggerFitToWindow = true;
        }
        else {
            mModeName = QString();
            setWindowTitle(mPartName);
            mPartView->scene()->addSimpleText(tr("no modes"));
        }
    }
    else {
        setWindowTitle(mPartName + tr("!"));
        mPartView->scene()->addSimpleText(tr("invalid"));
    }

    if (mFrameNumber >= numFrames()){
        mFrameNumber = numFrames()-1;
    }

    setFrame(mFrameNumber);

	auto sceneRect = mPartView->scene()->sceneRect();
	sceneRect.adjust(-sceneRect.width() * 0.125/2, -sceneRect.height() * 0.125/2, sceneRect.width() * 0.125/2, sceneRect.height() * 0.125/2);
	mPartView->setSceneRect(sceneRect);

	if (triggerFitToWindow) fitToWindow();
}

void PartWidget::setFrame(int f){
    int oldf = mFrameNumber;    
    mFrameNumber = f;
    showFrame(mFrameNumber);
    if (oldf!=f){
        emit(frameChanged(mFrameNumber));
    }
}

void PartWidget::setPlaybackSpeedMultiplier(int index, float value){
    mPlaybackSpeedMultiplierIndex = index;
    mPlaybackSpeedMultiplier = value;
}

void PartWidget::setDrawToolType(DrawToolType type){
    mDrawToolType=type;

    if (mClipboardItem!=nullptr){
        mPartView->scene()->removeItem(mClipboardItem);
        delete mClipboardItem;
        mClipboardItem = nullptr;
    }

    if (mCopyRectItem){
        mPartView->scene()->removeItem(mCopyRectItem);
        delete mCopyRectItem;
        mCopyRectItem = nullptr;
    }

    switch(mDrawToolType){
        case kDrawToolPaint: {
            mPartView->setCursor(QCursor(QPixmap::fromImage(QImage(":/icon/icons/tool_draw.png")),8,8));
            break;
        }
        case kDrawToolEraser: {
            mEraserColour = GlobalPreferences().backgroundColour;
            mPartView->setCursor(QCursor(QPixmap::fromImage(QImage(":/icon/icons/tool_erase.png")),8,8));
            break;
        }
        case kDrawToolPickColour: {
            mPartView->setCursor(QCursor(QPixmap::fromImage(QImage(":/icon/icons/tool_pickcolour.png")),8,8));
            break;
        }
        case kDrawToolStamp: {
            // TODO: tint the icon
            QImage img = QImage(":/icon/icons/tool_draw.png");
            QPixmap pm = QPixmap::fromImage(img);
            mPartView->setCursor(pm);

            QImage cImage = QGuiApplication::clipboard()->image();
            if (cImage.isNull()){
                qDebug() << "Clipboard doesn't contain valid image!";
            }
            else {
                // qDebug() << "Clipboard contains image with dim " << cImage.width() << "x" << cImage.height();
                mClipboardImage = QPixmap::fromImage(cImage);
                mClipboardItem = mPartView->scene()->addPixmap(mClipboardImage);

                QPointF pt = mPartView->mapToScene(mMousePos.x(),mMousePos.y());
                QPointF snap = QPointF(floor(pt.x()),floor(pt.y()));
                mClipboardItem->setPos(snap);
            }

            break;
         }
        case kDrawToolCopy: {
            mPartView->setCursor(QCursor(QPixmap::fromImage(QImage(":/icon/icons/tool_draw.png")),8,8));
            break;
        }
        default: {
            mPartView->setCursor(QCursor(QPixmap::fromImage(QImage(":/icon/icons/tool_none.png")),8,8));
            break;
        }
    }
}

void PartWidget::setMode(const QString& mode){
	if (mModeName != mode) {
		mModeName = mode;
		updatePartFrames();
	}
}

void PartWidget::partNameChanged(const QString& newPartName){
    mPartName = newPartName;
	if (mModeName.isEmpty()) {
		setWindowTitle(mPartName);
	}
	else {
		setWindowTitle(mPartName + tr(": ") + mModeName + tr(""));
	}
}

void PartWidget::partFrameUpdated(AssetRef part, const QString& mode, int /*frame*/){
    if (part==mPartRef && mModeName==mode){
        // update ..
        updatePartFrames();
    }
}

void PartWidget::partFramesUpdated(AssetRef part, const QString& mode){
    if (part==mPartRef && mModeName==mode){
        // update ..
        updatePartFrames();
    }
}

void PartWidget::partNumPivotsUpdated(AssetRef part, const QString& mode){
    if (part==mPartRef && mModeName==mode){
        // update ..
        Part* part = PM()->getPart(mPartRef);
        Part::Mode& mode = part->modes[mModeName];
        mNumPivots = mode.numPivots;
        showFrame(mFrameNumber);
    }
}

void PartWidget::updatePropertiesOverlays(){
    // Parse the properties
    // trying to extra any info that we can draw into the scene
	for(auto* pi: mPropertyItems){
        mPartView->scene()->removeItem(pi);
        delete pi;
    }
	mPropertyItems.clear();

    QJsonParseError error;
    QJsonDocument propDoc = QJsonDocument::fromJson(mProperties.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || propDoc.isNull() || propDoc.isEmpty() || !propDoc.isObject()){
        // couldn't parse.. (or not object)
    }
    else {
		// const int strokeAlpha = 64;
		// const QColor strokeColour = mBackgroundColour.lightnessF() > 0.5 ? QColor(128, 0, 128, strokeAlpha) : QColor(255, 0, 255, strokeAlpha);
		// const QColor fontColour = mBackgroundColour.lightnessF() < 0.5 ? QColor(0, 0, 0, 255) : QColor(255, 255, 255, 255);

		const QColor strokeColour = mPropertiesColour;

		QFont labelFont ("monospace");
		const QColor labelColour = QColor(255,255,255); // QColor(strokeColour.red(), strokeColour.green(), strokeColour.blue(), 128);
		const QColor labelBgColour = mPropertiesColour;

		// font.setHintingPreference(QFont::PreferFullHinting);

        int index = 0;
        QJsonObject propObj = propDoc.object();
		bool validMode = true;
		
		// Filter rects by valid modes
		for (auto it = propObj.begin(); it != propObj.end(); it++) {
			if (it.key().endsWith("_mode")){
				validMode = it.value().toString() == mModeName;
				break;
			}
		}

        for(auto it = propObj.begin(); it!=propObj.end(); it++){
            QString key = it.key();
            if (key.contains("rect") && it.value().isArray() && validMode){
                // draw a rectangle over the sprite

                const QJsonArray& array = it.value().toArray();
                 if (array.size()==4){
                    double x = array.at(0).toDouble(0);
                    double y = array.at(1).toDouble(0);
                    double w = array.at(2).toDouble(1);
                    double h = array.at(3).toDouble(1);

					QPen pen = QPen(strokeColour, 0.05);
                    QBrush brush = Qt::NoBrush;
					mPropertyItems.append(mPartView->scene()->addRect(x, y, w, h, pen, brush));
					
					auto* label = mPartView->scene()->addSimpleText(key, labelFont);
					label->setScale(0.05 / 2);
					label->setPos(x, y - 0.45);
					label->setPen(Qt::NoPen);
					label->setBrush(labelColour);
					label->setZValue(10);
					mPropertyItems.append(label);

					auto* labelRect = mPartView->scene()->addRect(label->sceneBoundingRect(), Qt::NoPen, QBrush(labelBgColour));
					labelRect->setZValue(label->zValue() - 1);
					mPropertyItems.append(labelRect);
					
                    index++;
                 }
            }
        }
    }

	// Test: add bounds
	if (mPart && !mModeName.isEmpty() && mPart->modes.contains(mModeName)){
		auto& mode = mPart->modes[mModeName];
		auto& bounds = mode.bounds;
		bounds = {};
		for (auto img : mode.frames) {
			int cropLeft = img->width();
			int cropTop = img->height();
			int cropRight = 0;
			int cropBottom = 0;
			for (int x = 0; x < img->width(); ++x) {
				for (int y = 0; y < img->height(); ++y) {
					const bool opaque = (img->pixelColor(x, y).alpha() > 0);
					if (opaque) {
						if (cropLeft > x) cropLeft = x;
						if (cropRight < x) cropRight = x;
						if (cropTop > y) cropTop = y;
						if (cropBottom < y) cropBottom = y;
					}
				}
			}

			int left = cropLeft;
			int top = cropTop;
			int width = 1 + cropRight - cropLeft;
			int height = 1 + cropBottom - cropTop;
			
			QRect frameBounds { left, top, width, height };			
			if (frameBounds.isValid() && !frameBounds.isEmpty()) {
				if (bounds.isNull()) bounds = frameBounds;
				else bounds = bounds.united(frameBounds);
			}
		}

		if (bounds.isNull() || bounds.isEmpty()) {
			bounds = mode.frames[0]->rect();
		}

		// mPropertyItems.append(mPartView->scene()->addRect(bounds, Qt::NoPen, QColor(0, 0, 0, 64)));
	}
}

void PartWidget::partPropertiesChanged(AssetRef part){
    if (part==mPartRef){
        // update ..
        Part* part = PM()->getPart(mPartRef);
        mProperties = "{" + part->properties + "}";
        updatePropertiesOverlays();
        showFrame(mFrameNumber);
    }
}

void PartWidget::closeEvent(QCloseEvent *event)
{
    emit(closed(this));
    event->accept();
}

void PartWidget::resizeEvent(QResizeEvent *event) {
	QMdiSubWindow::resizeEvent(event);
	fitToWindow();
}

void PartWidget::focusInEvent(QFocusEvent *focusInEvent) {
	mPartView->setFocus();
}

void PartWidget::updateOverlay(){
    if (mOverlayPixmapItem){
        mOverlayPixmapItem->setPixmap(QPixmap::fromImage(*mOverlayImage));
    }
}

void PartWidget::setZoom(int z){
    mZoom = z;
	// TODO: Zoom in on mouse position
	QTransform tr = QTransform::fromScale(mZoom, mZoom);
    mPartView->setTransform(tr);
	// mPartView->setSceneRect(mPartView->scene()->sceneRect().translated(mPosition.x()/mZoom, mPosition.y()/mZoom));
    updateDropShadow();
    mPartView->update();
}

void PartWidget::setPenSize(int size){
    mPenSize = size;
	// Update
	emit(penChanged());
}

void PartWidget::setPenColour(QColor colour){
    mPenColour = colour;
}

void PartWidget::fitToWindow(){
    // use graphicsview fit to window
    if (mPartView){
        // mPartView->centerOn();
		// TODO: Compute a tighter bounds item
		if (mPart && !mModeName.isEmpty() && mPart->modes.contains(mModeName) && mPart->modes[mModeName].bounds.isValid()) {
			mPartView->fitInView(mPart->modes[mModeName].bounds.adjusted(-2, -2, 2, 2), Qt::KeepAspectRatio);
		}
		else {
			mPartView->fitInView(mBoundsItem, Qt::KeepAspectRatio);
		}
        QTransform transform = mPartView->transform();
        QPoint scale = transform.map(QPoint(1,1));
        mZoom = floor(scale.x());
        setZoom(mZoom);		
		mViewportCenter = mPartView->mapToScene(mPartView->viewport()->rect().center());
        zoomChanged();
    }
}

void PartWidget::updateDropShadow(){
	const auto& prefs = GlobalPreferences();
    float opacity = prefs.showDropShadow ? prefs.dropShadowOpacity : 0.0f;
    float blur = prefs.dropShadowBlurRadius;
    float dx = prefs.dropShadowOffsetH;
	float dy = prefs.dropShadowOffsetV;
	QColor dropShadowColour = prefs.dropShadowColour;
	dropShadowColour.setAlphaF(opacity);

    if (mPartView){		
        for(auto* it: mPixmapItems){
            auto* effect = (QGraphicsDropShadowEffect*) it->graphicsEffect();
			if (effect) {
				effect->setOffset(dx * mZoom, dy * mZoom);
				effect->setColor(dropShadowColour);
				effect->setBlurRadius(blur * mZoom);
			}
        }
		
		showFrame(mFrameNumber); // To re-enable drop shadow
    }
}

void PartWidget::updateOnionSkinning(){	
	const auto& prefs = GlobalPreferences();
    if (mPartView){
		mOnionSkinningEnabled = prefs.showOnionSkinning;
		mOnionSkinningOpacity = prefs.onionSkinningOpacity;
        mOnionSkinningEnabledDuringPlayback = true;
        showFrame(mFrameNumber);
    }
}

void PartWidget::updatePivots(){
	const auto& prefs = GlobalPreferences();
	bool pivotsEnabled = prefs.showAnchors;
	bool pivotsEnabledDuringPlayback = true;
    if (mPartView){
        mPivotsEnabled = pivotsEnabled;
        mPivotsEnabledDuringPlayback = pivotsEnabledDuringPlayback;
        showFrame(mFrameNumber);
    }
}

static float S_PER_UPDATE = 1.f/60;
void PartWidget::play(bool play){
    if (!play){
        mIsPlaying = false;
        if (mAnimationTimer){
            mAnimationTimer->stop();
        }
    }
    else {
        if (mAnimationTimer==nullptr){
            mAnimationTimer=new QTimer(this);
            connect(mAnimationTimer, SIGNAL(timeout()), this, SLOT(updateAnimation()));
        }
        mSecondsPassedSinceLastFrame = 0;
        mAnimationTimer->start(S_PER_UPDATE*1000);
        mIsPlaying = true;
    }
}

void PartWidget::stop(){
    mIsPlaying = false;
    if (mAnimationTimer){
        mAnimationTimer->stop();
        mFrameNumber = 0;
        showFrame(mFrameNumber);
        emit(frameChanged(mFrameNumber));
    }
}

void PartWidget::updateAnimation(){
    mSecondsPassedSinceLastFrame += mPlaybackSpeedMultiplier*S_PER_UPDATE;
    bool updated = false;
    while (mSecondsPassedSinceLastFrame > mSPF){
        updated = true;
        mSecondsPassedSinceLastFrame -= mSPF;
        mFrameNumber = (mFrameNumber+1)%mPixmapItems.size();
    }
    if (updated){
        showFrame(mFrameNumber);
        emit(frameChanged(mFrameNumber));
    }
}

void PartWidget::partViewMousePressEvent(QMouseEvent *event){
    mLastPoint = event->pos();
    bool left = event->button()==Qt::LeftButton;
    bool right = event->button()==Qt::RightButton;
    bool middle = event->button()==Qt::MiddleButton;

    if (mScribbling && right && (mDrawToolType==kDrawToolPaint || mDrawToolType==kDrawToolEraser)){
        mScribbling = false;
        // Cancel        
        mOverlayImage->fill(0x00FFFFFF);
        updateOverlay();
    }
    else if (left && mDrawToolType==kDrawToolPaint){
        drawLineTo(mLastPoint);
        mScribbling = true;
    }
    else if (left && mDrawToolType==kDrawToolEraser){
        eraseLineTo(mLastPoint);
        mScribbling = true;
    }
    else if (left && mDrawToolType==kDrawToolCopy){
        // eraseLineTo(mLastPoint);
        // record top point
        mTopLeftPoint = event->pos();
        mScribbling = true;

        QPointF pt = mPartView->mapToScene(event->pos().x(),event->pos().y());
        QPen pen = QPen(QColor(255,0,255), 0.1, Qt::DashLine);
        QBrush brush = Qt::NoBrush;
        mCopyRectItem = mPartView->scene()->addRect(floor(pt.x()),floor(pt.y()),1,1,pen,brush);
    }
    else if (left && mDrawToolType==kDrawToolFill){
        QPointF pt = mPartView->mapToScene(event->pos().x(),event->pos().y());
        QPoint pi(floor(pt.x()),floor(pt.y()));

        // Perform fill
        const auto img = mPart->modes[mModeName].frames.at(mFrameNumber);

        if (pi.x()>=0 && pi.x()<img->width() && pi.y()>=0 && pi.y()<img->height()){
            QImage fillPattern = img->copy(); // (QSize(img->width(),img->height()), QImage::Format_ARGB32);
            // fillPattern.fill(0x00000000);

            QRgb targetColour = fillPattern.pixel(pi.x(),pi.y());
            QRgb replacementColour = mPenColour.rgba();

            if (targetColour!=replacementColour){
                QQueue<QPoint> q;
                q.enqueue(pi);
                while(!q.isEmpty()){
                    QPoint p = q.dequeue();
                   //  qDebug() << p;
                    if (p.x()>=0 && p.x()<fillPattern.width() && p.y()>=0 && p.y()<fillPattern.height()){
                        QRgb c = fillPattern.pixel(p);
                        if (c==targetColour){
                            fillPattern.setPixel(p, replacementColour);
                            q.enqueue(QPoint(p.x()+1,p.y()));
                            q.enqueue(QPoint(p.x()-1,p.y()));
                            q.enqueue(QPoint(p.x(),p.y()+1));
                            q.enqueue(QPoint(p.x(),p.y()-1));
                        }
                    }
                }

                TryCommand(new CDrawOnPart(mPartRef, mModeName, mFrameNumber, fillPattern, QPoint(0,0)));
            }
        }
    }
    else if ((left&&mDrawToolType==kDrawToolPickColour) || right){
        // select colour under pen
        QPointF pt = mPartView->mapToScene(event->pos().x(),event->pos().y());
        selectColourUnderPoint(pt);
    }
    else if (left&&mDrawToolType==kDrawToolStamp){
        // stamp
        if (mClipboardItem!=nullptr){
            TryCommand(new CDrawOnPart(mPartRef, mModeName, mFrameNumber, mClipboardItem->pixmap().toImage().copy(),  mClipboardItem->pos().toPoint()));
        }
    }
    else if (middle){
		mLastViewportCenter = mViewportCenter;
		// mLastViewOrigin = mPartView->transform().map(QPointF(0, 0));
        mMovingCanvas = true;
    }
}

void PartWidget::partViewMouseMoveEvent(QMouseEvent *event){
    mMousePos = event->pos();

    bool left = event->buttons()&Qt::LeftButton;
    bool right = event->buttons()&Qt::RightButton;
    bool middle = event->buttons()&Qt::MiddleButton;

    if (left && mScribbling){
        if (mDrawToolType==kDrawToolPaint){
            drawLineTo(mMousePos);
        }
        else if (mDrawToolType==kDrawToolEraser){
            eraseLineTo(mMousePos);
        }
        else if (mDrawToolType==kDrawToolCopy && mCopyRectItem!=nullptr){
            QPointF pt = mPartView->mapToScene(event->pos().x(),event->pos().y());
            QRectF rect = mCopyRectItem->rect();
            int w = floor(pt.x()) - rect.x();
            int h = floor(pt.y()) - rect.y();
            mCopyRectItem->setRect(rect.x(),rect.y(),w,h);
        }
    }
    else if (right){

    }
    else if (middle && mMovingCanvas){
        QPointF diff = (mLastPoint - mMousePos) / mZoom;
		mViewportCenter = mLastViewportCenter + diff;
		mPartView->centerOn(mViewportCenter);
		mPartView->update();
    }

    if (mClipboardItem!=nullptr){
        // Set proper position (mapped and snapped)
        QPointF pt = mPartView->mapToScene(mMousePos.x(),mMousePos.y());
        QPointF snap = QPointF(floor(pt.x()),floor(pt.y()));
        mClipboardItem->setPos(snap);
    }
}

void PartWidget::partViewMouseReleaseEvent(QMouseEvent *event){
    if (event->button() == Qt::LeftButton && mScribbling) {
        mScribbling = false;
        if (mDrawToolType==kDrawToolPaint){
            drawLineTo(event->pos());

            // Create the drawIntoCommand and clear the image and pixmap item
            // TODO: clip the image to save space..
            // Can use Pixmap->boundingRect()...
            TryCommand(new CDrawOnPart(mPartRef, mModeName, mFrameNumber, *mOverlayImage, mOverlayPixmapItem->pos().toPoint()));
            mOverlayImage->fill(0x00FFFFFF);
            updateOverlay();
        }
        else if (mDrawToolType==kDrawToolEraser){
            eraseLineTo(event->pos());
            TryCommand(new CEraseOnPart(mPartRef, mModeName, mFrameNumber, *mOverlayImage, mOverlayPixmapItem->pos().toPoint()));
            mOverlayImage->fill(0x00FFFFFF);
            updateOverlay();
        }
        else if (mDrawToolType==kDrawToolCopy){
            // qDebug() << "Copied rect in image to clipboard";

            QRectF rect = mCopyRectItem->rect();
            const auto img = mPart->modes[mModeName].frames.at(mFrameNumber);
            QImage subImg = img->copy(rect.x(),rect.y(),rect.width(),rect.height());
            if (subImg.isNull()){
                qDebug() << "Can't copy image region";
            }
            else {
//                QMimeData* mimeData = new QMimeData();
//                QByteArray data;
//                QBuffer buffer(&data);
//                buffer.open(QIODevice::WriteOnly);
//                subImg.save(&buffer, "PNG");
//                buffer.close();
//                mimeData->setData("application/x-qt-windows-mime;value=\"PNG\"", data);
//                QGuiApplication::clipboard()->setMimeData( mimeData );

                // NB: This doesn't work with GIMP for some reason
                QGuiApplication::clipboard()->setImage(subImg);

                //QGuiApplication::clipboard()->setPixmap(QPixmap::fromImage(subImg));
            }
            // QGuiApplication::clipboard()->setImage(subImg, QClipboard::Selection);

            // delete rect
            if (mCopyRectItem!=nullptr){
                mPartView->scene()->removeItem(mCopyRectItem);
                delete mCopyRectItem;
                mCopyRectItem = nullptr;
            }
        }
    }
    else if (event->button()==Qt::MiddleButton && mMovingCanvas){
        mMovingCanvas = false;
    }
}

void PartWidget::partViewWheelEvent(QWheelEvent *event){
    {
        QPoint pt = event->angleDelta();
        if (pt.y()>0){
            if (zoom()<48){
                // zoom in proportional to current zoom..
                int nz = std::min(48, zoom()*2);
                setZoom(nz);
                emit(zoomChanged());
            }
        }
        else if (pt.y()<0) {
            if (zoom()>1){
                int nz = std::max(1, zoom()/2);
                setZoom(nz);
                emit(zoomChanged());
            }
        }
    }
}

void PartWidget::partViewKeyPressEvent(QKeyEvent *event){
    QPointF coordf = mPartView->mapToScene(mMousePos.x(),mMousePos.y());
    QPoint coord(floor(coordf.x()), floor(coordf.y()));

    bool updatedAnchorOrPivots = false;

    switch (event->key()){
    case Qt::Key_A: {
        updatedAnchorOrPivots = true;
        mAnchors.replace(mFrameNumber, coord);
        break;
    }
    case Qt::Key_1: {
        updatedAnchorOrPivots = true;
        if (mNumPivots>0) mPivots[0].replace(mFrameNumber, coord);
        break;
    }
    case Qt::Key_2: {
        updatedAnchorOrPivots = true;
        if (mNumPivots>1) mPivots[1].replace(mFrameNumber, coord);
        break;
    }
    case Qt::Key_3: {
        updatedAnchorOrPivots = true;
        if (mNumPivots>2) mPivots[2].replace(mFrameNumber, coord);
        break;
    }
    case Qt::Key_4: {
        updatedAnchorOrPivots = true;
        if (mNumPivots>3) mPivots[3].replace(mFrameNumber, coord);
        break;
    }
    case Qt::Key_E: {
        emit(selectNextMode());
        break;
    }
    case Qt::Key_W: {
        emit(selectPreviousMode());
        break;
    }
    case Qt::Key_D: {
        // next frame (and stop if playing)
        if (mIsPlaying) play(false);
        if (mFrameNumber >= (numFrames()-1)){
            setFrame(0);
        }
        else {
            setFrame(mFrameNumber+1);
        }
        break;
    }
    case Qt::Key_S: {
        if (mIsPlaying) stop();
        if (mFrameNumber > 0){
            setFrame(mFrameNumber-1);
        }
        else {
            setFrame(numFrames()-1);
        }
        break;
    }
    case Qt::Key_Space: {
        // Toggle play/pause
        if (mIsPlaying) stop();
        else play(true);

        emit (playActivated(mIsPlaying));
        break;
    }
    }

    if (updatedAnchorOrPivots){
        QPoint a = mAnchors.at(mFrameNumber);
        QPoint p1 = (mNumPivots>0)?mPivots[0].at(mFrameNumber):QPoint(0,0);
        QPoint p2 = (mNumPivots>1)?mPivots[1].at(mFrameNumber):QPoint(0,0);
        QPoint p3 = (mNumPivots>2)?mPivots[2].at(mFrameNumber):QPoint(0,0);
        QPoint p4 = (mNumPivots>3)?mPivots[3].at(mFrameNumber):QPoint(0,0);
        TryCommand(new CUpdateAnchorAndPivots(mPartRef, mModeName, mFrameNumber, a, p1, p2, p3, p4));
    }
}

void PartWidget::drawLineTo(const QPoint &endPoint)
{
    float offset = penSize()/2.;
    QPainter painter(mOverlayImage);
    painter.setPen(QPen(penColour(), penSize(), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));

    if (endPoint==mLastPoint){
        QPointF lastPointImageCoords = mPartView->mapToScene(mLastPoint.x()+offset,mLastPoint.y()+offset);
        lastPointImageCoords.setX(floor(lastPointImageCoords.x()));
        lastPointImageCoords.setY(floor(lastPointImageCoords.y()));
        painter.drawPoint(lastPointImageCoords);
    }
    else {
        // map the points
        QPointF lastPointImageCoords = mPartView->mapToScene(mLastPoint.x()+offset,mLastPoint.y()+offset);
        lastPointImageCoords.setX(floor(lastPointImageCoords.x()));
        lastPointImageCoords.setY(floor(lastPointImageCoords.y()));
        QPointF endPointImageCoords = mPartView->mapToScene(endPoint.x()+offset,endPoint.y()+offset);
        endPointImageCoords.setX(floor(endPointImageCoords.x()));
        endPointImageCoords.setY(floor(endPointImageCoords.y()));
        painter.drawLine(lastPointImageCoords, endPointImageCoords);
    }
    // modified
    updateOverlay();
    mLastPoint = endPoint;
}

void PartWidget::eraseLineTo(const QPoint &endPoint)
{
    float offset = penSize()/2.;
    QPainter painter(mOverlayImage);
    painter.setPen(QPen(mEraserColour, penSize(), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));

    // painter.setBrush(mBackgroundBrush);

    if (endPoint==mLastPoint){
        QPointF lastPointImageCoords = mPartView->mapToScene(mLastPoint.x()+offset,mLastPoint.y()+offset);
        lastPointImageCoords.setX(floor(lastPointImageCoords.x()));
        lastPointImageCoords.setY(floor(lastPointImageCoords.y()));
        painter.drawPoint(lastPointImageCoords);
    }
    else {
        // map the points
        QPointF lastPointImageCoords = mPartView->mapToScene(mLastPoint.x()+offset,mLastPoint.y()+offset);
        lastPointImageCoords.setX(floor(lastPointImageCoords.x()));
        lastPointImageCoords.setY(floor(lastPointImageCoords.y()));
        QPointF endPointImageCoords = mPartView->mapToScene(endPoint.x()+offset,endPoint.y()+offset);
        endPointImageCoords.setX(floor(endPointImageCoords.x()));
        endPointImageCoords.setY(floor(endPointImageCoords.y()));
        painter.drawLine(lastPointImageCoords, endPointImageCoords);
    }
    // modified
    updateOverlay();
    mLastPoint = endPoint;
}

void PartWidget::selectColourUnderPoint(QPointF pt){
    int px = floor(pt.x());
    int py = floor(pt.y());
    QGraphicsPixmapItem* pi = mPixmapItems.at(mFrameNumber%mPixmapItems.size());

    bool inBounds = px>=0 || px<pi->pixmap().width() || py>=0 || py<pi->pixmap().height();
    if (pi!=nullptr && inBounds){
        QImage img = pi->pixmap().toImage();
        QImage alpha = img.alphaChannel();
        QRgb rgb = img.pixel(pt.x(), pt.y());
        if (QColor(alpha.pixel(pt.x(),pt.y())).red()!=0){
            QColor colour(rgb);
            setPenColour(colour);
            setDrawToolType(kDrawToolPaint);
            emit(penChanged());
        }
        else {
            // Change to eraser
            setDrawToolType(kDrawToolEraser);
            emit(penChanged());
        }
    }
    else if (!inBounds){
        // Change to eraser
        setDrawToolType(kDrawToolEraser);
        emit(penChanged());
    }
}

void PartWidget::showFrame(int f){
    bool onion = mOnionSkinningEnabled && ((mIsPlaying&&mOnionSkinningEnabledDuringPlayback) || !mIsPlaying);
    bool pivots = mPivotsEnabled && ((mIsPlaying&&mPivotsEnabledDuringPlayback) || !mIsPlaying);

	// Drop shadow kills perf when zoomed in too much
	const bool disableDropShadow = (mIsPlaying && mZoom > 10) || (!mIsPlaying && mZoom > 50);

    for(int i=0;i<mPixmapItems.size();i++){
        QGraphicsPixmapItem* pi = mPixmapItems.at(i);
		auto* effect = pi->graphicsEffect();		
        if (effect) effect->setEnabled(false);        
		for (auto* it : mAnchorItems.at(i)) {
			it->hide();
		}
		for (int p = 0; p < MAX_PIVOTS; p++) {
			for (auto* it : mPivotItems[p].at(i)) {
				it->hide();
			}
		}
        if (i == f){
			pi->show();
            pi->setOpacity(1);
			if (!disableDropShadow && effect && static_cast<QGraphicsDropShadowEffect*>(effect)->color().alpha() > 0) effect->setEnabled(true);
            if (pivots){
				for (auto* it : mAnchorItems.at(i)) {
					it->show();
				}
				for (int p = 0; p < mNumPivots; p++) {
					for (auto* it : mPivotItems[p].at(i)) {
						it->show();
					}
				}
            }
        }
        else if (onion && ((i-f)==1 || (i-f)==-1)){
			pi->show();
            pi->setOpacity(mOnionSkinningOpacity);
        }
        else if (onion && ((i-f)==2 || (i-f)==-2)){
			pi->show();
            pi->setOpacity(mOnionSkinningOpacity /4);
        }
        else {
            pi->hide();
        }
    }
}

void PartWidget::updateBackgroundBrushes(){
	const auto& prefs = GlobalPreferences();
	auto secondaryColor = [](QColor colour1) {
		int h = colour1.hue();
		int s = colour1.saturation();
		int val = colour1.value();
		if (val < 50) {
			val += 10;
		}
		else if (val < 255 / 2) {
			val *= 1.05;
		}
		else if (val < (255 / 2 - 50)) {
			val /= 1.05;
		}
		else {
			val -= 10;
		}
		QColor colour2;
		colour2.setHsv(h, s, val);
		return colour2;
	};

    if (!prefs.backgroundCheckerboard){
		 mBackgroundColour = prefs.backgroundColour;
         mBackgroundBrush = QBrush(mBackgroundColour);
    }
    else {
		mBackgroundColour = prefs.backgroundColour;
		QColor backgroundColour2 = secondaryColor(mBackgroundColour);

        int w = 2;
        QImage img(w, w, QImage::Format_RGB32);
        for(int i=0;i<w;i++){
         for(int j=0;j<w;j++){
            bool d = (i/(w/2) + j/(w/2))%2==0;
            if (d) img.setPixel(i, j, mBackgroundColour.rgb());
            else img.setPixel(i, j, backgroundColour2.rgb());
         }
        }
        mBackgroundBrush = QBrush(img);
        mBackgroundBrush.setTransform(QTransform::fromScale(0.5,0.5));
    }

	mOutOfBoundsColour = QColor(160, 160, 160, 255); // Colour of mdi window

	// TODO: Compute properties colour?
	mPropertiesColour = QColor(255, 0, 255, 255);
}

////////////////////////////////////////////////
// Part View
////////////////////////////////////////////////

PartView::PartView(PartWidget *parent, QGraphicsScene *scene)
    :QGraphicsView(scene, parent),
      pw(parent)
{
    setMouseTracking(true);

	// setOptimizationFlags(QGraphicsView::OptimizationFlag::DontAdjustForAntialiasing);
    // setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);
	// setRenderHints(0); 
	// setViewport(new QOpenGLWidget(this));

    setCursor(QCursor(QPixmap::fromImage(QImage(":/icon/icons/tool_none.png")),8,8));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void PartView::scrollContentsBy(int sx, int sy)
{
    QGraphicsView::scrollContentsBy(sx,sy);
}

void PartView::mousePressEvent(QMouseEvent *event){
    pw->partViewMousePressEvent(event);
}

void PartView::mouseMoveEvent(QMouseEvent *event){
    pw->partViewMouseMoveEvent(event);
}

void PartView::mouseReleaseEvent(QMouseEvent *event){
    pw->partViewMouseReleaseEvent(event);
}

void PartView::wheelEvent(QWheelEvent *event){
    pw->partViewWheelEvent(event);
}

void PartView::keyPressEvent(QKeyEvent *event){
    pw->partViewKeyPressEvent(event);
}

void PartView::drawBackground(QPainter *painter, const QRectF &rect)
{
	painter->fillRect(rect, QBrush(pw->mOutOfBoundsColour));
    if (pw->mBoundsItem){
        // QPen pen = pw->mBoundsItem->pen();
        // pen.setColor(pw->mBoundsColour);
        pw->mBoundsItem->setPen(Qt::NoPen);
		pw->mBoundsItem->setBrush(pw->mBackgroundBrush);
    }
}
