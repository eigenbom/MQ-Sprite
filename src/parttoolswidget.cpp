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


PartToolsWidget::PartToolsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PartToolsWidget),
    mTarget(nullptr)
{
    ui->setupUi(this);

    mPenColour = QColor("black");
    mToolButtonColour = findChild<QToolButton*>("toolButtonColour");
    connect(mToolButtonColour, SIGNAL(clicked()), this, SLOT(chooseColour()));

    mLineEditColour = findChild<QLineEdit*>("lineEditColour");
    connect(mLineEditColour, SIGNAL(textEdited(QString)), this, SLOT(chooseColourByText(QString)));

    mTextEditProperties = findChild<QPlainTextEdit*>("textEditProperties");
    Q_ASSERT(mTextEditProperties);
    connect(mTextEditProperties, SIGNAL(textChanged()), this, SLOT(textPropertiesEdited()));

    // mPaletteView = findChild<PaletteView*>("paletteView");
    // connect(mPaletteView, SIGNAL(colourSelected(QColor)), this, SLOT(colourSelected(QColor)));

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

    mResizeModeDialog = new ResizeModeDialog(this);
    mResizeModeDialog->hide();
    connect(mResizeModeDialog, SIGNAL(accepted()), this, SLOT(resizeModeDialogAccepted()));
    mAnimatorWidget = findChild<AnimatorWidget*>("animatorWidget");

    /*
    // Disabled for now
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
    QString selectedPalette = settings.value("selected_palette", tr(":/palette/24bit.png")).toString();
    // qDebug() << "!" << selectedPalette << mComboBoxPalettes->findText(selectedPalette);
    mComboBoxPalettes->setCurrentIndex(mComboBoxPalettes->findText(selectedPalette));
    paletteActivated(selectedPalette);
 */

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
    mAnimatorWidget->setTargetPartWidget(p);

    // Disconnect and connect signals
    if (mTarget){
        // disconnect
        disconnect(findChild<QSlider*>("hSliderZoom"), SIGNAL(valueChanged(int)), mTarget, SLOT(setZoom(int)));
        disconnect(findChild<QToolButton*>("toolButtonFitToWindow"), SIGNAL(clicked()), mTarget, SLOT(fitToWindow()));
        disconnect(findChild<QSlider*>("hSliderPenSize"), SIGNAL(valueChanged(int)), mTarget, SLOT(setPenSize(int)));
        disconnect(mTarget, SIGNAL(penChanged()), this, SLOT(penChanged()));
        disconnect(mTarget, SIGNAL(zoomChanged()), this, SLOT(zoomChanged()));                
        disconnect(mTarget, SIGNAL(selectNextMode()), this, SLOT(selectNextMode()));
        disconnect(mTarget, SIGNAL(selectPreviousMode()), this, SLOT(selectPreviousMode()));

        mTarget = nullptr;
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

        QSlider* hSliderNumPivots = findChild<QSlider*>("hSliderNumPivots");
        hSliderNumPivots->setMaximum(MAX_PIVOTS);
        hSliderNumPivots->setValue(p->numPivots());

        // set properties
        Part* part = PM()->getPart(p->partRef());
        Q_ASSERT(part);

        mTextEditProperties->blockSignals(true);

        QTextCursor cursor = mTextEditProperties->textCursor();
        cursor.movePosition(QTextCursor::Start);
        mTextEditProperties->setPlainText(part->properties);
        mTextEditProperties->blockSignals(false);
        targetPartModesChanged();

        const Part::Mode& m = part->modes.value(p->modeName());
        mPushButtonModeSize->setText(QString("%1x%2").arg(m.width).arg(m.height));

        // connect
        connect(findChild<QSlider*>("hSliderZoom"), SIGNAL(valueChanged(int)), p, SLOT(setZoom(int)));
        connect(findChild<QToolButton*>("toolButtonFitToWindow"), SIGNAL(clicked()), p, SLOT(fitToWindow()));
        connect(findChild<QSlider*>("hSliderPenSize"), SIGNAL(valueChanged(int)), p, SLOT(setPenSize(int)));
        connect(p, SIGNAL(penChanged()), this, SLOT(penChanged()));
        connect(p, SIGNAL(zoomChanged()), this, SLOT(zoomChanged()));        
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
        TryCommand(new CChangePartProperties(mTarget->partRef(), mTextEditProperties->toPlainText()));
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

void PartToolsWidget::targetPartNumFramesChanged(){
    mAnimatorWidget->targetPartNumFramesChanged();
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
        // Update modes combobox
        mComboBoxModes->clear();
        Part* part = PM()->getPart(mTarget->partRef());
        if (part){
            QStringList list = part->modes.keys();
            mComboBoxModes->addItems(list);
            mComboBoxModes->setCurrentIndex(mComboBoxModes->findText(mTarget->modeName()));

            // Update size
            Part::Mode m = part->modes.value(mTarget->modeName());
            mPushButtonModeSize->setText(QString("%1x%2").arg(m.width).arg(m.height));            
        }

        targetPartNumPivotsChanged();

        mAnimatorWidget->targetPartModesChanged();
    }
}

void PartToolsWidget::targetPartPropertiesChanged(){
    if (mTarget){
        Part* part = PM()->getPart(mTarget->partRef());
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

void PartToolsWidget::setNumPivots(int p){
    if (mTarget){
        // Create command
        TryCommand(new CChangeNumPivots(mTarget->partRef(), mTarget->modeName(), p));
    }
}

void PartToolsWidget::modeActivated(QString mode){
    if (mTarget){
        mAnimatorWidget->stop();
        mTarget->setMode(mode);
        mAnimatorWidget->modeActivated(mode);

        // update num frames etc
        Part* part = PM()->getPart(mTarget->partRef());
        Part::Mode m = part->modes.value(mode);
        mPushButtonModeSize->setText(QString("%1x%2").arg(m.width).arg(m.height));

        targetPartNumPivotsChanged();
    }
}

void PartToolsWidget::addMode(){
    if (mTarget){
        mAnimatorWidget->stop();
        QString mode = mComboBoxModes->currentText();
        TryCommand(new CNewMode(mTarget->partRef(), mode));
    }
}

void PartToolsWidget::copyMode(){
    if (mTarget){
        mAnimatorWidget->stop();
        QString mode = mComboBoxModes->currentText();
        TryCommand(new CCopyMode(mTarget->partRef(), mode));
    }
}

void PartToolsWidget::deleteMode(){
    if (mTarget){
        mAnimatorWidget->stop();
        QString mode = mComboBoxModes->currentText();
        if (mComboBoxModes->count()>1){
            TryCommand(new CDeleteMode(mTarget->partRef(), mode));
        }
        else if (mComboBoxModes->count()==1){
            // Don't delete mode, but instead clear it..
            TryCommand(new CResetMode(mTarget->partRef(), mode));
        }
    }
}

void PartToolsWidget::renameMode(){
    if (mTarget){
        mAnimatorWidget->stop();
        QString currentMode = mComboBoxModes->currentText();
        bool ok;
        QString text = QInputDialog::getText(this, tr("Rename Mode"),
                                             tr("Rename ") + currentMode + ":", QLineEdit::Normal,
                                             currentMode, &ok);
        if (ok && !text.isEmpty()){
            TryCommand(new CRenameMode(mTarget->partRef(), currentMode, text));
        }
    }
}

void PartToolsWidget::resizeMode(){
    if (mTarget){
        mAnimatorWidget->stop();
        // Get current size
        QString currentMode = mComboBoxModes->currentText();
        Part* part = PM()->getPart(mTarget->partRef());
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
        TryCommand(new CChangeModeSize(mTarget->partRef(), currentMode, width, height, offsetx, offsety));
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
