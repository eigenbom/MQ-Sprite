#include "animatorwidget.h"
#include "ui_animatorwidget.h"
#include "partwidget.h"
#include "paletteview.h"
#include "commands.h"
#include "mainwindow.h"
#include <cmath>

#include <QtWidgets>
#include <QMessageBox>
#include <QInputDialog>

static const int NUM_PLAYBACK_SPEED_MULTIPLIERS = 7;
static float PLAYBACK_SPEED_MULTIPLIERS[NUM_PLAYBACK_SPEED_MULTIPLIERS] = {1./8,1./4,1./2,1,2,4,8};
static const char* PLAYBACK_SPEED_MULTIPLIER_LABELS[NUM_PLAYBACK_SPEED_MULTIPLIERS] = {"1/8","1/4","1/2","1","2","4","8"};

AnimatorWidget::AnimatorWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::AnimatorWidget),
    mTarget(nullptr)
{
    ui->setupUi(this);

    connect(findChild<QToolButton*>("toolButtonAddFrame"), SIGNAL(clicked()), this, SLOT(addFrame()));
    connect(findChild<QToolButton*>("toolButtonCopyFrame"), SIGNAL(clicked()), this, SLOT(copyFrame()));
    connect(findChild<QToolButton*>("toolButtonDeleteFrame"), SIGNAL(clicked()), this, SLOT(deleteFrame()));
    connect(findChild<QToolButton*>("toolButtonPlay"), SIGNAL(clicked(bool)), this, SLOT(play(bool)));
    connect(findChild<QToolButton*>("toolButtonStop"), SIGNAL(clicked()), this, SLOT(stop()));
    connect(findChild<QToolButton*>("toolButtonNextFrame"), SIGNAL(clicked()), this, SLOT(goToNextFrame()));
    connect(findChild<QToolButton*>("toolButtonPrevFrame"), SIGNAL(clicked()), this, SLOT(goToPrevFrame()));
    connect(findChild<QToolButton*>("toolButtonFirstFrame"), SIGNAL(clicked()), this, SLOT(goToFirstFrame()));
    connect(findChild<QToolButton*>("toolButtonLastFrame"), SIGNAL(clicked()), this, SLOT(goToLastFrame()));
    connect(findChild<QSlider*>("hSliderTimeLine"), SIGNAL(valueChanged(int)), this, SLOT(goToFrame(int)));

    // mPushButtonModeFPS = findChild<QPushButton*>("pushButtonModeFPS");
    // connect(mPushButtonModeFPS, SIGNAL(clicked()), this, SLOT(setModeFPS()));

	/*
    mHSliderPlaybackSpeedMultiplier = findChild<QSlider*>("hSliderPlaybackSpeedMultiplier");
    mLineEditPlaybackSpeedMultiplier = findChild<QLineEdit*>("lineEditPlaybackSpeedMultiplier");
    connect(mHSliderPlaybackSpeedMultiplier, SIGNAL(valueChanged(int)), this, SLOT(setPlaybackSpeedMultiplier(int)));
	*/

    this->setEnabled(false);
}

AnimatorWidget::~AnimatorWidget()
{
    delete ui;
}

PartWidget* AnimatorWidget::targetPartWidget(){
    return mTarget;
}

void AnimatorWidget::setTargetPartWidget(PartWidget* p){
    // Disconnect and connect signals
    if (mTarget){
        // disconnect
        disconnect(mTarget, SIGNAL(frameChanged(int)), this, SLOT(frameChanged(int)));
        disconnect(mTarget, SIGNAL(playActivated(bool)), this, SLOT(playActivated(bool)));
        mTarget = nullptr;
    }
    if (p){
        mTarget = p;

        findChild<QToolButton*>("toolButtonPlay")->setChecked(p->isPlaying());
        QSlider* hSliderTimeLine = findChild<QSlider*>("hSliderTimeLine");
        hSliderTimeLine->setMaximum(p->numFrames()-1);
        hSliderTimeLine->setValue(p->frame());
        findChild<QLineEdit*>("lineEditFrameNumber")->setText(QString::number(p->frame()+1) + "/" + QString::number(p->numFrames()));

        // set properties
        Part* part = PM()->getPart(p->partRef());
        Q_ASSERT(part);

        targetPartModesChanged();

        Part::Mode m = part->modes.value(p->modeName());
        // mPushButtonModeFPS->setText(QString::number(m.framesPerSecond));

        int psmi = p->playbackSpeedMultiplierIndex();
        if (psmi==-1){
            p->setPlaybackSpeedMultiplier(3, 1.0);
            psmi = 3;
        }

        // mHSliderPlaybackSpeedMultiplier->setValue(psmi);
        // mLineEditPlaybackSpeedMultiplier->setText(tr("x") + PLAYBACK_SPEED_MULTIPLIER_LABELS[psmi]);

        // connect
        connect(p, SIGNAL(frameChanged(int)), this, SLOT(frameChanged(int)));
        connect(p, SIGNAL(playActivated(bool)), this, SLOT(playActivated(bool)));
        this->setEnabled(true);
    }
    else {
        this->setEnabled(false);
    }
}

void AnimatorWidget::frameChanged(int f){
    if (mTarget){
        findChild<QLineEdit*>("lineEditFrameNumber")->setText(QString::number(mTarget->frame()+1) + "/" + QString::number(mTarget->numFrames()));

        QSlider* sl = findChild<QSlider*>("hSliderTimeLine");
        bool wha = sl->blockSignals(true);
        sl->setValue(f);
        sl->blockSignals(wha);
    }
}

void AnimatorWidget::targetPartNumFramesChanged(){
    // update number of frames
    if (mTarget){
        QSlider* hSliderTimeLine = findChild<QSlider*>("hSliderTimeLine");
        hSliderTimeLine->setMaximum(mTarget->numFrames()-1);
        hSliderTimeLine->setValue(mTarget->frame());
        findChild<QLineEdit*>("lineEditFrameNumber")->setText(QString::number(mTarget->frame()+1) + "/" + QString::number(mTarget->numFrames()));
    }
}

void AnimatorWidget::targetPartModesChanged(){
    if (mTarget){
        // Update combobox
        Part* part = PM()->getPart(mTarget->partRef());
        if (part){
            // Update fps
            // Part::Mode m = part->modes.value(mTarget->modeName());
            // mPushButtonModeFPS->setText(QString::number(m.framesPerSecond));
        }

        targetPartNumFramesChanged();
        int psmi = mTarget->playbackSpeedMultiplierIndex();
        if (psmi==-1){
            mTarget->setPlaybackSpeedMultiplier(3, 1.0);
            psmi = 3;
        }
        // mHSliderPlaybackSpeedMultiplier->setValue(psmi);
        // mLineEditPlaybackSpeedMultiplier->setText(tr("x") + PLAYBACK_SPEED_MULTIPLIER_LABELS[psmi]);
    }
}

void AnimatorWidget::addFrame(){
    if (mTarget){
        // qDebug() << "addFrame";
        stop();

        if (TryCommand(new CNewFrame(mTarget->partRef(), mTarget->modeName(), mTarget->numFrames()))){
            goToFrame(mTarget->numFrames()-1);
        }
    }
}

void AnimatorWidget::copyFrame(){
    if (mTarget){
        stop();
        if (TryCommand(new CCopyFrame(mTarget->partRef(), mTarget->modeName(), mTarget->frame()))){
            goToFrame(mTarget->numFrames()-1);
        }
    }
}

void AnimatorWidget::deleteFrame(){
    if (mTarget){
        stop();
        // Do it...
        if (mTarget->numFrames()>1){
            TryCommand(new CDeleteFrame(mTarget->partRef(), mTarget->modeName(), mTarget->frame()));
        }
        else {
            // TODO: show error in status bar
        }

    }
}

void AnimatorWidget::play(bool b){
    if (mTarget){
        mTarget->play(b);
    }
}

void AnimatorWidget::playActivated(bool b){
    findChild<QToolButton*>("toolButtonPlay")->setChecked(b);
}

void AnimatorWidget::stop(){
    if (mTarget){
        if (mTarget->isPlaying()){
            mTarget->stop();
            // uncheck play button
            findChild<QToolButton*>("toolButtonPlay")->setChecked(false);
        }
    }
}

void AnimatorWidget::goToLastFrame(){
    if (mTarget){
        goToFrame(mTarget->numFrames()-1);
    }
}

void AnimatorWidget::goToFirstFrame(){
    if (mTarget){
        goToFrame(0);
    }
}

void AnimatorWidget::goToNextFrame(){
    if (mTarget){
        int f = std::min(mTarget->frame()+1, mTarget->numFrames()-1);
        goToFrame(f);
    }
}

void AnimatorWidget::goToPrevFrame(){
    if (mTarget){
        int f = std::max(mTarget->frame()-1, 0);
        goToFrame(f);
    }
}

void AnimatorWidget::goToFrame(int f){
    if (mTarget){
        // if playing then stop..
        if (mTarget->isPlaying()){
            stop();
        }
        mTarget->setFrame(f);
    }
}

void AnimatorWidget::modeActivated(QString mode){
    if (mTarget){
        // update num frames etc
        // Part* part = PM()->getPart(mTarget->partRef());
        // Part::Mode m = part->modes.value(mode);
        // mPushButtonModeFPS->setText(QString::number(m.framesPerSecond));
        targetPartNumFramesChanged();
    }
}

void AnimatorWidget::setModeFPS(){
    if (mTarget){
        bool ok;
        int fps = QInputDialog::getInt(this, "Set FPS", tr("Set FPS:"), mPushButtonModeFPS->text().toInt(),1,600,1,&ok);
        if (ok){
            TryCommand(new CChangeModeFPS(mTarget->partRef(), mTarget->modeName(), fps));
        }
    }
}

void AnimatorWidget::setPlaybackSpeedMultiplier(int i){
    Q_ASSERT(i>=0 && i<NUM_PLAYBACK_SPEED_MULTIPLIERS);
    if (mTarget){
        float playbackSpeed = PLAYBACK_SPEED_MULTIPLIERS[i];
        const char* label = PLAYBACK_SPEED_MULTIPLIER_LABELS[i];
        // mLineEditPlaybackSpeedMultiplier->setText(tr("x") + label);
        mTarget->setPlaybackSpeedMultiplier(i, playbackSpeed);
    }
}
