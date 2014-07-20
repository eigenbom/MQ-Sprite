#include "parttoolswidget.h"
#include "ui_parttoolswidget.h"
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

PartToolsWidget::PartToolsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PartToolsWidget),
    mTarget(NULL)
{
    ui->setupUi(this);

    mPenColour = QColor("black");
    mToolButtonColour = findChild<QToolButton*>("toolButtonColour");
    connect(mToolButtonColour, SIGNAL(clicked()), this, SLOT(chooseColour()));

    mLineEditColour = findChild<QLineEdit*>("lineEditColour");
    connect(mLineEditColour, SIGNAL(textEdited(QString)), this, SLOT(chooseColourByText(QString)));

    mTextEditProperties = findChild<QPlainTextEdit*>("textEditProperties");
    connect(mTextEditProperties, SIGNAL(textChanged()), this, SLOT(textPropertiesEdited()));

    mPaletteView = findChild<PaletteView*>("paletteView");
    connect(mPaletteView, SIGNAL(colourSelected(QColor)), this, SLOT(colourSelected(QColor)));

    QPixmap px(16, 16);
    px.fill(mPenColour);
    mToolButtonColour->setIcon(px);

    QToolButton* toolButtonDraw = findChild<QToolButton*>("toolButtonDraw");
    QToolButton* toolButtonErase = findChild<QToolButton*>("toolButtonErase");
    QToolButton* toolButtonPickColour = findChild<QToolButton*>("toolButtonPickColour");
    QToolButton* toolButtonStamp = findChild<QToolButton*>("toolButtonStamp");
    QToolButton* toolButtonCopy = findChild<QToolButton*>("toolButtonCopy");
    QToolButton* toolButtonFill = findChild<QToolButton*>("toolButtonFill");

    mActionDraw = findChild<QAction*>("actionDraw");
    mActionErase = findChild<QAction*>("actionErase");
    mActionPickColour = findChild<QAction*>("actionPickColour");
    mActionStamp = findChild<QAction*>("actionStamp");
    mActionCopy = findChild<QAction*>("actionCopy");
    mActionFill = findChild<QAction*>("actionFill");

    connect(mActionDraw, SIGNAL(triggered()), this, SLOT(setToolTypeDraw()));
    connect(mActionErase, SIGNAL(triggered()), this, SLOT(setToolTypeErase()));
    connect(mActionPickColour, SIGNAL(triggered()), this, SLOT(setToolTypePickColour()));
    connect(mActionStamp, SIGNAL(triggered()), this, SLOT(setToolTypeStamp()));
    connect(mActionCopy, SIGNAL(triggered()), this, SLOT(setToolTypeCopy()));
    connect(mActionFill, SIGNAL(triggered()), this, SLOT(setToolTypeFill()));

    toolButtonDraw->setDefaultAction(mActionDraw);
    toolButtonErase->setDefaultAction(mActionErase);
    toolButtonPickColour->setDefaultAction(mActionPickColour);
    toolButtonStamp->setDefaultAction(mActionStamp);
    toolButtonCopy->setDefaultAction(mActionCopy);
    toolButtonFill->setDefaultAction(mActionFill);

    QActionGroup* aGroup = new QActionGroup(this);
    aGroup->addAction(mActionDraw);
    aGroup->addAction(mActionErase);
    aGroup->addAction(mActionPickColour);
    aGroup->addAction(mActionStamp);
    aGroup->addAction(mActionCopy);
    aGroup->addAction(mActionFill);
    mActionDraw->setChecked(true);

    connect(findChild<QToolButton*>("toolButtonPlay"), SIGNAL(clicked(bool)), this, SLOT(play(bool)));
    connect(findChild<QToolButton*>("toolButtonStop"), SIGNAL(clicked()), this, SLOT(stop()));
    connect(findChild<QToolButton*>("toolButtonNextFrame"), SIGNAL(clicked()), this, SLOT(goToNextFrame()));
    connect(findChild<QToolButton*>("toolButtonPrevFrame"), SIGNAL(clicked()), this, SLOT(goToPrevFrame()));
    connect(findChild<QToolButton*>("toolButtonFirstFrame"), SIGNAL(clicked()), this, SLOT(goToFirstFrame()));
    connect(findChild<QToolButton*>("toolButtonLastFrame"), SIGNAL(clicked()), this, SLOT(goToLastFrame()));

    connect(findChild<QSlider*>("hSliderTimeLine"), SIGNAL(valueChanged(int)), this, SLOT(goToFrame(int)));

    connect(findChild<QToolButton*>("toolButtonAddFrame"), SIGNAL(clicked()), this, SLOT(addFrame()));
    connect(findChild<QToolButton*>("toolButtonCopyFrame"), SIGNAL(clicked()), this, SLOT(copyFrame()));
    connect(findChild<QToolButton*>("toolButtonDeleteFrame"), SIGNAL(clicked()), this, SLOT(deleteFrame()));

    connect(findChild<QToolButton*>("toolButtonAddMode"), SIGNAL(clicked()), this, SLOT(addMode()));
    connect(findChild<QToolButton*>("toolButtonCopyMode"), SIGNAL(clicked()), this, SLOT(copyMode()));
    connect(findChild<QToolButton*>("toolButtonDeleteMode"), SIGNAL(clicked()), this, SLOT(deleteMode()));

    connect(findChild<QSlider*>("hSliderNumPivots"), SIGNAL(valueChanged(int)), this, SLOT(setNumPivots(int)));

    mComboBoxModes = findChild<QComboBox*>("comboBoxModes");
    connect(mComboBoxModes, SIGNAL(activated(QString)), this, SLOT(modeActivated(QString)));
    // connect(mComboBoxModes, SIGNAL(editTextChanged(QString)), this, SLOT(currentModeRenamed(QString)));
    // connect(mComboBoxModes->lineEdit(), SIGNAL(editingFinished()), SLOT(editingFinished()));

    connect(findChild<QToolButton*>("toolButtonRenameMode"), SIGNAL(clicked()), this, SLOT(renameMode()));

    mPushButtonModeSize = findChild<QPushButton*>("pushButtonModeSize");
    connect(mPushButtonModeSize, SIGNAL(clicked()), this, SLOT(resizeMode()));

    mPushButtonModeFPS = findChild<QPushButton*>("pushButtonModeFPS");
    connect(mPushButtonModeFPS, SIGNAL(clicked()), this, SLOT(setModeFPS()));

    mResizeModeDialog = new ResizeModeDialog(this);
    mResizeModeDialog->hide();
    connect(mResizeModeDialog, SIGNAL(accepted()), this, SLOT(resizeModeDialogAccepted()));

    mHSliderPlaybackSpeedMultiplier = findChild<QSlider*>("hSliderPlaybackSpeedMultiplier");
    mLineEditPlaybackSpeedMultiplier = findChild<QLineEdit*>("lineEditPlaybackSpeedMultiplier");
    connect(mHSliderPlaybackSpeedMultiplier, SIGNAL(valueChanged(int)), this, SLOT(setPlaybackSpeedMultiplier(int)));

    connect(findChild<QToolButton*>("toolButtonAddPalette"), SIGNAL(clicked()), this, SLOT(addPalette()));
    connect(findChild<QToolButton*>("toolButtonDeletePalette"), SIGNAL(clicked()), this, SLOT(deletePalette()));
    mComboBoxPalettes = findChild<QComboBox*>("comboBoxPalette");
    connect(mComboBoxPalettes, SIGNAL(activated(QString)), this, SLOT(paletteActivated(QString)));

    // Load palette filenames...
    // (And verify them)

    mDefaultPalettes << ":/palette/db16.png";
    mDefaultPalettes << ":/palette/24bit.png";
    mDefaultPalettes << ":/palette/EGA.png";
    mDefaultPalettes << ":/palette/NES.png";

    QSettings settings;
    QStringList paletteList = settings.value("palette_list", mDefaultPalettes).toStringList();

    foreach(QString str, mDefaultPalettes){
        if (!paletteList.contains(str)){
            paletteList.prepend(str);
        }
    }
    foreach(const QString str, paletteList){
        mComboBoxPalettes->addItem(str);
    }

    QString selectedPalette = settings.value("selected_palette", tr(":/palette/db16.png")).toString();
    // qDebug() << "!" << selectedPalette << mComboBoxPalettes->findText(selectedPalette);
    mComboBoxPalettes->setCurrentIndex(mComboBoxPalettes->findText(selectedPalette));
    paletteActivated(selectedPalette);

    this->setEnabled(false);
}

PartToolsWidget::~PartToolsWidget()
{
    delete ui;
}

PartWidget* PartToolsWidget::targetPartWidget(){
    return mTarget;
}

void PartToolsWidget::setTargetPartWidget(PartWidget* p){
    // Disconnect and connect signals
    if (mTarget){
        // disconnect
        disconnect(findChild<QSlider*>("hSliderZoom"), SIGNAL(valueChanged(int)), mTarget, SLOT(setZoom(int)));
        disconnect(findChild<QToolButton*>("toolButtonFitToWindow"), SIGNAL(clicked()), mTarget, SLOT(fitToWindow()));
        disconnect(findChild<QSlider*>("hSliderPenSize"), SIGNAL(valueChanged(int)), mTarget, SLOT(setPenSize(int)));
        disconnect(mTarget, SIGNAL(penChanged()), this, SLOT(penChanged()));
        disconnect(mTarget, SIGNAL(zoomChanged()), this, SLOT(zoomChanged()));
        disconnect(mTarget, SIGNAL(frameChanged(int)), this, SLOT(frameChanged(int)));
        disconnect(mTarget, SIGNAL(playActivated(bool)), this, SLOT(playActivated(bool)));
        disconnect(mTarget, SIGNAL(selectNextMode()), this, SLOT(selectNextMode()));
        disconnect(mTarget, SIGNAL(selectPreviousMode()), this, SLOT(selectPreviousMode()));

        mTarget = NULL;
    }
    if (p){
        mTarget = p;

        // update all widgets
        findChild<QSlider*>("hSliderZoom")->setValue(p->zoom());
        p->setPenSize(findChild<QSlider*>("hSliderPenSize")->value()); // nb: pen size is stored in toolwidget
        p->setPenColour(mPenColour);
        if (mActionDraw->isChecked()) p->setDrawToolType(kDrawToolPaint);
        else if (mActionErase->isChecked()) p->setDrawToolType(kDrawToolEraser);
        else if (mActionPickColour->isChecked()) p->setDrawToolType(kDrawToolPickColour);
        findChild<QToolButton*>("toolButtonPlay")->setChecked(p->isPlaying());
        QSlider* hSliderTimeLine = findChild<QSlider*>("hSliderTimeLine");
        hSliderTimeLine->setMaximum(p->numFrames()-1);
        hSliderTimeLine->setValue(p->frame());
        findChild<QLineEdit*>("lineEditFrameNumber")->setText(QString::number(p->frame()+1) + "/" + QString::number(p->numFrames()));

        QSlider* hSliderNumPivots = findChild<QSlider*>("hSliderNumPivots");
        hSliderNumPivots->setMaximum(MAX_PIVOTS);
        hSliderNumPivots->setValue(p->numPivots());

        // set properties
        mTextEditProperties->blockSignals(true);
        mTextEditProperties->setPlainText(p->properties());
        mTextEditProperties->blockSignals(false);

        targetPartModesChanged();

        Part* part = PM()->getPart(p->partName());
        Part::Mode m = part->modes.value(p->modeName());
        mPushButtonModeSize->setText(QString("%1x%2").arg(m.width).arg(m.height));
        mPushButtonModeFPS->setText(QString::number(m.framesPerSecond));

        int psmi = p->playbackSpeedMultiplierIndex();
        if (psmi==-1){
            p->setPlaybackSpeedMultiplier(3, 1.0);
            psmi = 3;
        }

        mHSliderPlaybackSpeedMultiplier->setValue(psmi);
        mLineEditPlaybackSpeedMultiplier->setText(tr("x") + PLAYBACK_SPEED_MULTIPLIER_LABELS[psmi]);

        // connect
        connect(findChild<QSlider*>("hSliderZoom"), SIGNAL(valueChanged(int)), p, SLOT(setZoom(int)));
        connect(findChild<QToolButton*>("toolButtonFitToWindow"), SIGNAL(clicked()), p, SLOT(fitToWindow()));
        connect(findChild<QSlider*>("hSliderPenSize"), SIGNAL(valueChanged(int)), p, SLOT(setPenSize(int)));
        connect(p, SIGNAL(penChanged()), this, SLOT(penChanged()));
        connect(p, SIGNAL(zoomChanged()), this, SLOT(zoomChanged()));
        connect(p, SIGNAL(frameChanged(int)), this, SLOT(frameChanged(int)));        
        connect(p, SIGNAL(playActivated(bool)), this, SLOT(playActivated(bool)));
        connect(p, SIGNAL(selectNextMode()), this, SLOT(selectNextMode()));
        connect(p, SIGNAL(selectPreviousMode()), this, SLOT(selectPreviousMode()));

        this->setEnabled(true);

    }
    else {
        this->setEnabled(false);
    }
}

void PartToolsWidget::chooseColour(){
    mPenColour = QColorDialog::getColor(mPenColour);
    QPixmap px(16, 16);
    px.fill(mPenColour);
    mToolButtonColour->setIcon(px);

    // Update the line edit
    mLineEditColour->setText(mPenColour.name());

    if (mTarget){
        mTarget->setPenColour(mPenColour);
        if (mTarget->drawToolType()!=kDrawToolPaint && mTarget->drawToolType()!=kDrawToolFill){
            mTarget->setDrawToolType(kDrawToolPaint);
        }
        penChanged();
    }
}

void PartToolsWidget::chooseColourByText(QString str){
    // qDebug() << str;

    if (QColor::isValidColor(str)){
        mPenColour = QColor(str);

        QPixmap px(16, 16);
        px.fill(mPenColour);
        mToolButtonColour->setIcon(px);

        // Update the line edit
        // mLineEditColour->setText(mPenColour.name());

        if (mTarget){
            mTarget->setPenColour(mPenColour);
            if (mTarget->drawToolType()!=kDrawToolPaint && mTarget->drawToolType()!=kDrawToolFill){
                mTarget->setDrawToolType(kDrawToolPaint);
            }
            // penChanged();
        }
    }
}

void PartToolsWidget::colourSelected(QColor colour){
    if (mTarget){
        mPenColour = colour;
        mTarget->setPenColour(mPenColour);
        QPixmap px(16, 16);
        px.fill(mPenColour);        
        mToolButtonColour->setIcon(px);

        // Update the line edit
        mLineEditColour->setText(mPenColour.name());

        if (mTarget->drawToolType()!=kDrawToolPaint && mTarget->drawToolType()!=kDrawToolFill){
            mTarget->setDrawToolType(kDrawToolPaint);
        }
        penChanged();
    }
}

void PartToolsWidget::textPropertiesEdited(){
    if (mTarget){
        ChangePropertiesCommand* command = new ChangePropertiesCommand(mTarget->partName(), mTextEditProperties->toPlainText());
        if (command->ok){
            MainWindow::Instance()->undoStack()->push(command);
        }
        else {
            delete command;
        }
    }
}

void PartToolsWidget::penChanged(){
    if (mTarget){
        findChild<QSlider*>("hSliderPenSize")->setValue(mTarget->penSize());
        mPenColour = mTarget->penColour();
        QPixmap px(16, 16);
        px.fill(mPenColour);
        mToolButtonColour->setIcon(px);

        // Update the line edit
        mLineEditColour->setText(mPenColour.name());

        switch(mTarget->drawToolType()){
        case kDrawToolPaint: mActionDraw->trigger(); break;
        case kDrawToolEraser: mActionErase->trigger(); break;
        case kDrawToolPickColour: mActionPickColour->trigger(); break;
        default: break;
        }
    }
}

void PartToolsWidget::zoomChanged(){
    if (mTarget){
        findChild<QSlider*>("hSliderZoom")->setValue(mTarget->zoom());
    }
}

void PartToolsWidget::frameChanged(int f){
    if (mTarget){
        findChild<QLineEdit*>("lineEditFrameNumber")->setText(QString::number(mTarget->frame()+1) + "/" + QString::number(mTarget->numFrames()));

        QSlider* sl = findChild<QSlider*>("hSliderTimeLine");
        bool wha = sl->blockSignals(true);
        sl->setValue(f);
        sl->blockSignals(wha);
    }
}

void PartToolsWidget::targetPartNumFramesChanged(){
    // update number of frames
    if (mTarget){
        QSlider* hSliderTimeLine = findChild<QSlider*>("hSliderTimeLine");
        hSliderTimeLine->setMaximum(mTarget->numFrames()-1);
        hSliderTimeLine->setValue(mTarget->frame());
        findChild<QLineEdit*>("lineEditFrameNumber")->setText(QString::number(mTarget->frame()+1) + "/" + QString::number(mTarget->numFrames()));
    }
}

void PartToolsWidget::targetPartNumPivotsChanged(){
    // update number of pivots
    if (mTarget){
        QSlider* hSliderTimeLine = findChild<QSlider*>("hSliderNumPivots");
         bool wha = hSliderTimeLine->blockSignals(true);
        hSliderTimeLine->setValue(mTarget->numPivots());
        hSliderTimeLine->blockSignals(wha);
    }
}

void PartToolsWidget::targetPartModesChanged(){
    if (mTarget){
        // Update combobox
        mComboBoxModes->clear();
        Part* part = PM()->getPart(mTarget->partName());
        if (part){
            QStringList list = part->modes.keys();
            mComboBoxModes->addItems(list);
            mComboBoxModes->setCurrentIndex(mComboBoxModes->findText(mTarget->modeName()));

            // Update size
            Part::Mode m = part->modes.value(mTarget->modeName());
            mPushButtonModeSize->setText(QString("%1x%2").arg(m.width).arg(m.height));
            mPushButtonModeFPS->setText(QString::number(m.framesPerSecond));
        }

        targetPartNumFramesChanged();
        targetPartNumPivotsChanged();

        int psmi = mTarget->playbackSpeedMultiplierIndex();
        if (psmi==-1){
            mTarget->setPlaybackSpeedMultiplier(3, 1.0);
            psmi = 3;
        }
        mHSliderPlaybackSpeedMultiplier->setValue(psmi);
        mLineEditPlaybackSpeedMultiplier->setText(tr("x") + PLAYBACK_SPEED_MULTIPLIER_LABELS[psmi]);
    }
}

void PartToolsWidget::targetPartPropertiesChanged(){
    if (mTarget){
        Part* part = PM()->getPart(mTarget->partName());
        if (part){            
            int cursorPos = mTextEditProperties->textCursor().position();
            mTextEditProperties->blockSignals(true);
            mTextEditProperties->setPlainText(part->properties);            
            mTextEditProperties->blockSignals(false);

            QTextCursor cursor = mTextEditProperties->textCursor();
            cursor.setPosition(cursorPos, QTextCursor::MoveAnchor);
            mTextEditProperties->setTextCursor(cursor);
        }
    }
}

void PartToolsWidget::setToolTypeDraw(){
    if (mTarget){
        mTarget->setDrawToolType(kDrawToolPaint);
    }
}

void PartToolsWidget::setToolTypeErase(){
    if (mTarget){
        mTarget->setDrawToolType(kDrawToolEraser);
    }
}

void PartToolsWidget::setToolTypePickColour(){
    if (mTarget){
        mTarget->setDrawToolType(kDrawToolPickColour);
    }
}

void PartToolsWidget::setToolTypeStamp(){
    if (mTarget){
        mTarget->setDrawToolType(kDrawToolStamp);
    }
}

void PartToolsWidget::setToolTypeCopy(){
    if (mTarget){
        mTarget->setDrawToolType(kDrawToolCopy);
    }
}

void PartToolsWidget::setToolTypeFill(){
    if (mTarget){
        mTarget->setDrawToolType(kDrawToolFill);
    }
}

void PartToolsWidget::addFrame(){
    if (mTarget){
        // qDebug() << "addFrame";
        stop();
        NewFrameCommand* command = new NewFrameCommand(mTarget->partName(), mTarget->modeName(), mTarget->numFrames());
        if (command->ok){
            MainWindow::Instance()->undoStack()->push(command);
            goToFrame(mTarget->numFrames()-1);
        }
        else {
            // TODO: Show error in status bar?
            delete command;
        }
    }
}

void PartToolsWidget::copyFrame(){
    if (mTarget){
        stop();
        CopyFrameCommand* command = new CopyFrameCommand(mTarget->partName(), mTarget->modeName(), mTarget->frame());
        if (command->ok){
            MainWindow::Instance()->undoStack()->push(command);
            goToFrame(mTarget->numFrames()-1);
        }
        else {
            // TODO: Show error in status bar?
            delete command;
        }
    }
}

void PartToolsWidget::deleteFrame(){
    if (mTarget){
        stop();
        // Do it...
        if (mTarget->numFrames()>1){
            DeleteFrameCommand* command = new DeleteFrameCommand(mTarget->partName(), mTarget->modeName(), mTarget->frame());
            if (command->ok){
                MainWindow::Instance()->undoStack()->push(command);
            }
            else {
                // TODO: Show error in status bar?
                delete command;
            }
        }
        else {
            // TODO: show error in status bar
        }

    }
}

void PartToolsWidget::play(bool b){
    if (mTarget){
        mTarget->play(b);
    }
}

void PartToolsWidget::playActivated(bool b){
    findChild<QToolButton*>("toolButtonPlay")->setChecked(b);
}

void PartToolsWidget::stop(){
    if (mTarget){
        if (mTarget->isPlaying()){
            mTarget->stop();
            // uncheck play button
            findChild<QToolButton*>("toolButtonPlay")->setChecked(false);
        }
    }
}

void PartToolsWidget::goToLastFrame(){
    if (mTarget){
        goToFrame(mTarget->numFrames()-1);
    }
}

void PartToolsWidget::goToFirstFrame(){
    if (mTarget){
        goToFrame(0);
    }
}

void PartToolsWidget::goToNextFrame(){
    if (mTarget){
        int f = std::min(mTarget->frame()+1, mTarget->numFrames()-1);
        goToFrame(f);
    }
}

void PartToolsWidget::goToPrevFrame(){
    if (mTarget){
        int f = std::max(mTarget->frame()-1, 0);
        goToFrame(f);
    }
}

void PartToolsWidget::goToFrame(int f){
    if (mTarget){
        // if playing then stop..
        if (mTarget->isPlaying()){
            stop();
        }
        mTarget->setFrame(f);
    }
}

void PartToolsWidget::setNumPivots(int p){
    if (mTarget){
        // Create command
        ChangeNumPivotsCommand* command = new ChangeNumPivotsCommand(mTarget->partName(), mTarget->modeName(), p);
        if (command->ok){
            MainWindow::Instance()->undoStack()->push(command);
        }
        else {
            delete command;
        }
    }
}

void PartToolsWidget::modeActivated(QString mode){
    if (mTarget){
        stop();
        mTarget->setMode(mode);

        // update num frames etc
        Part* part = PM()->getPart(mTarget->partName());
        Part::Mode m = part->modes.value(mode);
        mPushButtonModeSize->setText(QString("%1x%2").arg(m.width).arg(m.height));
        mPushButtonModeFPS->setText(QString::number(m.framesPerSecond));
        targetPartNumFramesChanged();
        targetPartNumPivotsChanged();
    }
}

void PartToolsWidget::addMode(){
    if (mTarget){
        stop();
        QString mode = mComboBoxModes->currentText();
        NewModeCommand* command = new NewModeCommand(mTarget->partName(), mode);
        if (command->ok){
            MainWindow::Instance()->undoStack()->push(command);
        }
        else {
            delete command;
        }
    }
}

void PartToolsWidget::copyMode(){
    if (mTarget){
        stop();
        QString mode = mComboBoxModes->currentText();
        CopyModeCommand* command = new CopyModeCommand(mTarget->partName(), mode);
        if (command->ok){
            MainWindow::Instance()->undoStack()->push(command);
        }
        else {
            delete command;
        }

    }
}

void PartToolsWidget::deleteMode(){
    if (mTarget){
        stop();
        QString mode = mComboBoxModes->currentText();
        if (mComboBoxModes->count()>1){
            DeleteModeCommand* command = new DeleteModeCommand(mTarget->partName(), mode);
            if (command->ok){
                MainWindow::Instance()->undoStack()->push(command);
            }
            else {
                delete command;
            }
        }
        else if (mComboBoxModes->count()==1){
            // Don't delete mode, but instead clear it..
            ResetModeCommand* command = new ResetModeCommand(mTarget->partName(), mode);
            if (command->ok){
                MainWindow::Instance()->undoStack()->push(command);
            }
            else {
                delete command;
            }
        }
    }
}

void PartToolsWidget::renameMode(){
    if (mTarget){
        stop();
        QString currentMode = mComboBoxModes->currentText();
        bool ok;
        QString text = QInputDialog::getText(this, tr("Rename Mode"),
                                             tr("Rename ") + currentMode + ":", QLineEdit::Normal,
                                             currentMode, &ok);
        if (ok && !text.isEmpty()){
            RenameModeCommand* command = new RenameModeCommand(mTarget->partName(), currentMode, text);
            if (command->ok){
                MainWindow::Instance()->undoStack()->push(command);
            }
            else {
                delete command;
            }
        }
    }
}

void PartToolsWidget::resizeMode(){
    if (mTarget){
        stop();
        // Get current size
        QString currentMode = mComboBoxModes->currentText();
        Part* part = PM()->getPart(mTarget->partName());
        Part::Mode m = part->modes.value(currentMode);
        mResizeModeDialog->mLineEditWidth->setText(QString::number(m.width));
        mResizeModeDialog->mLineEditHeight->setText(QString::number(m.height));
        mResizeModeDialog->show();
    }
}

void PartToolsWidget::resizeModeDialogAccepted(){
    if (mTarget){
        // qDebug() << "resizeModeDialogAccepted";

        int width = mResizeModeDialog->mLineEditWidth->text().toInt();
        int height = mResizeModeDialog->mLineEditHeight->text().toInt();
        int offsetx = mResizeModeDialog->mLineEditOffsetX->text().toInt();
        int offsety = mResizeModeDialog->mLineEditOffsetY->text().toInt();
        QString currentMode = mComboBoxModes->currentText();
        ChangeModeSizeCommand* command = new ChangeModeSizeCommand(mTarget->partName(), currentMode, width, height, offsetx, offsety);
        if (command->ok){
            MainWindow::Instance()->undoStack()->push(command);
        }
        else {
            // qDebug() << "OI!";
            delete command;
        }
    }
}

void PartToolsWidget::setModeFPS(){
    if (mTarget){
        QString currentMode = mComboBoxModes->currentText();
        bool ok;
        int fps = QInputDialog::getInt(this, "Set FPS", tr("Set FPS:"), mPushButtonModeFPS->text().toInt(),1,600,1,&ok);
        if (ok){
            ChangeModeFPSCommand* command = new ChangeModeFPSCommand(mTarget->partName(), currentMode, fps);
            if (command->ok){
                MainWindow::Instance()->undoStack()->push(command);
            }
            else {
                delete command;
            }
        }
    }
}

void PartToolsWidget::setPlaybackSpeedMultiplier(int i){
    Q_ASSERT(i>=0 && i<NUM_PLAYBACK_SPEED_MULTIPLIERS);
    if (mTarget){
        float playbackSpeed = PLAYBACK_SPEED_MULTIPLIERS[i];
        const char* label = PLAYBACK_SPEED_MULTIPLIER_LABELS[i];
        mLineEditPlaybackSpeedMultiplier->setText(tr("x") + label);
        mTarget->setPlaybackSpeedMultiplier(i, playbackSpeed);
    }
}

void PartToolsWidget::addPalette(){
    QString file = QFileDialog::getOpenFileName(this, tr("Select Palette"), QString(), tr("Images (*.png *.jpg *.xpm)"));
    if (!file.isNull()){
        // Add palette
        if (QFile(file).exists()){
            QSettings settings;
            QStringList paletteList = settings.value("palette_list", QStringList()).toStringList();
            if (!(paletteList.contains(file))){
                paletteList.push_back(file);
                settings.setValue("palette_list", paletteList);
                // then add it to the combo box
                mComboBoxPalettes->addItem(file);
                mComboBoxPalettes->setCurrentIndex(mComboBoxPalettes->count()-1);
                paletteActivated(file);
            }
        }
    }
}

void PartToolsWidget::deletePalette(){
    QString file = mComboBoxPalettes->currentText();
    if (mDefaultPalettes.contains(file)) return; // can't delete default palette
    else {
        QSettings settings;
        QStringList paletteList = settings.value("palette_list", QStringList()).toStringList();
        if (paletteList.contains(file)){
            paletteList.removeAll(file);
            settings.setValue("palette_list", paletteList);
            // then remove it from the combo box
            mComboBoxPalettes->removeItem(mComboBoxPalettes->currentIndex());
            mComboBoxPalettes->setCurrentIndex(0);
            paletteActivated(mComboBoxPalettes->itemText(0));
        }
    }
}

void PartToolsWidget::paletteActivated(QString str){
    // qDebug() << "paletteActivated: " << str;
    mPaletteView->loadPalette(str);
    QSettings settings;
    settings.setValue(tr("selected_palette"), QVariant(str));
}

void PartToolsWidget::selectNextMode(){
    int i = mComboBoxModes->currentIndex();
    if (mComboBoxModes->count()>1){
        i = (i+1)%mComboBoxModes->count();
        mComboBoxModes->setCurrentIndex(i);
        modeActivated(mComboBoxModes->currentText());
    }
}

void PartToolsWidget::selectPreviousMode(){
    int i = mComboBoxModes->currentIndex();
    if (mComboBoxModes->count()>1){
        i = (i+mComboBoxModes->count()-1)%mComboBoxModes->count();
        mComboBoxModes->setCurrentIndex(i);
        modeActivated(mComboBoxModes->currentText());
    }
}
