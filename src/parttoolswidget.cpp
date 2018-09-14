#include "parttoolswidget.h"
#include "ui_parttoolswidget.h"

#include "commands.h"
#include "paletteview.h"
#include "partwidget.h"
#include "mainwindow.h"
#include "modelistwidget.h"
#include <QtWidgets>
#include <QMessageBox>
#include <QInputDialog>
#include <QListWidget>
#include <QToolButton>
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QStandardItemModel>
#include <QTreeView>
#include <QDebug>
#include <QMimeData>
#include <cmath>

PartToolsWidget::PartToolsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PartToolsWidget),
    mTarget(nullptr),
	mCurrentMode {}
{
    ui->setupUi(this);

    mPenColour = QColor("#000000");
    mToolButtonColour = findChild<QToolButton*>("toolButtonColour");
    connect(mToolButtonColour, SIGNAL(clicked()), this, SLOT(chooseColour()));
	mToolButtonColour->setToolTip(mPenColour.name());

    mTextEditProperties = findChild<QPlainTextEdit*>("textEditProperties");
    connect(mTextEditProperties, SIGNAL(textChanged()), this, SLOT(textPropertiesEdited()));
	mTextEditProperties->setPlaceholderText(tr("\"emit\": true,\n\"wiggle\": 0.5,\n\"display_name\":\"worm\""));

    QPixmap px(mToolButtonColour->iconSize());
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

	connect(findChild<QSpinBox*>("spinBoxNumPivots"), SIGNAL(valueChanged(int)), this, SLOT(setNumPivots(int)));

	mListWidgetModes = findChild<ModeListWidget*>("listWidgetModes");
	mListWidgetModes->clear();

	connect(mListWidgetModes, &QListWidget::itemChanged, [this](QListWidgetItem* item) {
		this->modeEditTextChanged(item->text());
	});
		
	connect(mListWidgetModes, &QListWidget::itemSelectionChanged, [this]() {
		auto* list = this->mListWidgetModes;
		for (auto* item : list->selectedItems()) {
			this->modeActivated(item->text());
		}
	});

	connect(findChild<QSpinBox*>("spinBoxFramerate"), SIGNAL(valueChanged(int)), this, SLOT(setFramerate(int)));

    mResizeModeDialog = new ResizeModeDialog(this);
    mResizeModeDialog->hide();
    connect(mResizeModeDialog, SIGNAL(accepted()), this, SLOT(resizeModeDialogAccepted()));
    mAnimatorWidget = findChild<AnimatorWidget*>("animatorWidget");

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
		mCurrentMode.clear();
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
		
		findChild<QSpinBox*>("spinBoxNumPivots")->setValue(p->numPivots());

        // set properties
        Part* part = PM()->getPart(p->partRef());
        Q_ASSERT(part);

        mTextEditProperties->blockSignals(true);

        QTextCursor cursor = mTextEditProperties->textCursor();
        cursor.movePosition(QTextCursor::Start);
        mTextEditProperties->setPlainText(part->properties);
        mTextEditProperties->blockSignals(false);
        targetPartModesChanged();

		updateProperties(part, p->modeName());

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
    QPixmap px(mToolButtonColour->iconSize());
    px.fill(mPenColour);	
    mToolButtonColour->setIcon(px);
	mToolButtonColour->setToolTip(mPenColour.name());

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

        QPixmap px(mToolButtonColour->iconSize());
        px.fill(mPenColour);
        mToolButtonColour->setIcon(px);
		mToolButtonColour->setToolTip(mPenColour.name());

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
        QPixmap px(mToolButtonColour->iconSize());
        px.fill(mPenColour);        
        mToolButtonColour->setIcon(px);

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
        QPixmap px(mToolButtonColour->iconSize());
        px.fill(mPenColour);
        mToolButtonColour->setIcon(px);

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
		auto* numPivotsWidget = findChild<QSpinBox*>("spinBoxNumPivots");
		bool wha = numPivotsWidget->blockSignals(true);
		numPivotsWidget->setValue(mTarget->numPivots());
		numPivotsWidget->blockSignals(wha);
    }
}

void PartToolsWidget::targetPartModesChanged(){
    if (mTarget){		
        mListWidgetModes->clear();
		mCurrentMode = mTarget->modeName();

        Part* part = PM()->getPart(mTarget->partRef());
        if (part){
            QStringList list = part->modes.keys();			
			mListWidgetModes->addItems(list);
			for (int i = 0; i < mListWidgetModes->count(); ++i)
			{
				auto* item = mListWidgetModes->item(i);
				item->setFlags(item->flags() | Qt::ItemIsEditable);
			}
			mListWidgetModes->blockSignals(true);
			auto items = mListWidgetModes->findItems(mCurrentMode, Qt::MatchExactly);
			mListWidgetModes->clearSelection();
			for (auto* item : items) {
				mListWidgetModes->setItemSelected(item, true);
				mListWidgetModes->setCurrentItem(item);
			}
			mListWidgetModes->blockSignals(false);

			updateProperties(part, mTarget->modeName());
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
		mCurrentMode = mode;

		mListWidgetModes->blockSignals(true);
		auto items = mListWidgetModes->findItems(mode, Qt::MatchExactly);
		mListWidgetModes->clearSelection();
		for (auto* item: items) {
			mListWidgetModes->setItemSelected(item, true);
			mListWidgetModes->setCurrentItem(item);
		}
		mListWidgetModes->blockSignals(false);

        mAnimatorWidget->stop();
        mTarget->setMode(mode);
        mAnimatorWidget->modeActivated(mode);

        // update properties
        Part* part = PM()->getPart(mTarget->partRef());
		updateProperties(part, mode);
        targetPartNumPivotsChanged();
    }
}

void PartToolsWidget::modeEditTextChanged(const QString& text) {
	if (mTarget && !mCurrentMode.isEmpty() && mCurrentMode != text && !text.isEmpty()) {
		mAnimatorWidget->stop();
		TryCommand(new CRenameMode(mTarget->partRef(), mCurrentMode, text));
	}
}

void PartToolsWidget::addMode(){
    if (mTarget){
        mAnimatorWidget->stop();
        TryCommand(new CNewMode(mTarget->partRef(), mCurrentMode));		
    }
}

void PartToolsWidget::copyMode(){
    if (mTarget){
        mAnimatorWidget->stop();
        TryCommand(new CCopyMode(mTarget->partRef(), mCurrentMode));
    }
}

void PartToolsWidget::deleteMode(){
    if (mTarget){
        mAnimatorWidget->stop();
		auto* item = mListWidgetModes->currentItem();
		if (item) {
			QString mode = item->text();
			if (mListWidgetModes->count() == 1) {
				TryCommand(new CResetMode(mTarget->partRef(), mode));
			}
			else {
				TryCommand(new CDeleteMode(mTarget->partRef(), mode));
			}
		}        
    }
}

void PartToolsWidget::resizeMode(){
    if (mTarget){
        mAnimatorWidget->stop();
        Part* part = PM()->getPart(mTarget->partRef());
        Part::Mode m = part->modes.value(mCurrentMode);
        mResizeModeDialog->mLineEditWidth->setText(QString::number(m.width));
        mResizeModeDialog->mLineEditHeight->setText(QString::number(m.height));
        mResizeModeDialog->show();
    }
}

void PartToolsWidget::resizeModeDialogAccepted(){
    if (mTarget){
        int width   = mResizeModeDialog->mLineEditWidth->text().toInt();
        int height  = mResizeModeDialog->mLineEditHeight->text().toInt();
        int offsetx = mResizeModeDialog->mLineEditOffsetX->text().toInt();
        int offsety = mResizeModeDialog->mLineEditOffsetY->text().toInt();
        TryCommand(new CChangeModeSize(mTarget->partRef(), mCurrentMode, width, height, offsetx, offsety));
    }
}

void PartToolsWidget::selectNextMode(){		
	if (mListWidgetModes->count() > 1) {
		int row = mListWidgetModes->currentIndex().row();
		row = (row + 1) % mListWidgetModes->count();
		auto nextMode = mListWidgetModes->item(row)->text();
		modeActivated(nextMode);
	}
}

void PartToolsWidget::selectPreviousMode(){
	if (mListWidgetModes->count() > 1) {
		int row = mListWidgetModes->currentIndex().row();
		row = (row + mListWidgetModes->count() - 1) % mListWidgetModes->count();
		auto nextMode = mListWidgetModes->item(row)->text();
		modeActivated(nextMode);
	}
}

void PartToolsWidget::setFramerate(int frameRate) {
	TryCommand(new CChangeModeFPS(mTarget->partRef(), mTarget->modeName(), frameRate));
}

void PartToolsWidget::updateProperties(Part* part, const QString& mode){
	const auto& m = part->modes.value(mode);
	findChild<QLabel*>("labelModeSize")->setText(QString("%1x%2").arg(m.width).arg(m.height));
	findChild<QSpinBox*>("spinBoxFramerate")->setValue(m.framesPerSecond);
}

