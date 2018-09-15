#include "animationwidget.h"
#include "ui_animationwidget.h"

#include "commands.h"
#include "partwidget.h"
#include "resizemodedialog.h"

static const int NUM_PLAYBACK_SPEED_MULTIPLIERS = 7;
static float PLAYBACK_SPEED_MULTIPLIERS[NUM_PLAYBACK_SPEED_MULTIPLIERS] = { 1. / 8,1. / 4,1. / 2,1,2,4,8 };
static const char* PLAYBACK_SPEED_MULTIPLIER_LABELS[NUM_PLAYBACK_SPEED_MULTIPLIERS] = { "1/8","1/4","1/2","1","2","4","8" };

AnimationWidget::AnimationWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AnimationWidget)
{
    ui->setupUi(this);

	connect(findChild<QToolButton*>("toolButtonAddMode"), SIGNAL(clicked()), this, SLOT(addMode()));
	connect(findChild<QToolButton*>("toolButtonCopyMode"), SIGNAL(clicked()), this, SLOT(copyMode()));
	connect(findChild<QToolButton*>("toolButtonDeleteMode"), SIGNAL(clicked()), this, SLOT(deleteMode()));
	connect(findChild<QSpinBox*>("spinBoxNumPivots"), SIGNAL(valueChanged(int)), this, SLOT(setNumPivots(int)));
	connect(findChild<QSpinBox*>("spinBoxFramerate"), SIGNAL(valueChanged(int)), this, SLOT(setFramerate(int)));

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


	mResizeModeDialog = new ResizeModeDialog(this);
	mResizeModeDialog->hide();
	connect(mResizeModeDialog, SIGNAL(accepted()), this, SLOT(resizeModeDialogAccepted()));
	// mAnimatorWidget = findChild<AnimatorWidget*>("animatorWidget");
	this->setEnabled(false);
}

AnimationWidget::~AnimationWidget()
{
	delete ui;
}

PartWidget* AnimationWidget::targetPartWidget() {
	return mTarget;
}

void AnimationWidget::setTargetPartWidget(PartWidget* p) {
	// mAnimatorWidget->setTargetPartWidget(p);

	// Disconnect and connect signals
	if (mTarget) {
		disconnect(mTarget, SIGNAL(selectNextMode()), this, SLOT(selectNextMode()));
		disconnect(mTarget, SIGNAL(selectPreviousMode()), this, SLOT(selectPreviousMode()));
		disconnect(mTarget, SIGNAL(frameChanged(int)), this, SLOT(frameChanged(int)));
		disconnect(mTarget, SIGNAL(playActivated(bool)), this, SLOT(playActivated(bool)));

		mTarget = nullptr;
		mCurrentMode.clear();
	}
	if (p) {
		mTarget = p;
		Part* part = PM()->getPart(p->partRef());

		findChild<QSpinBox*>("spinBoxNumPivots")->setValue(p->numPivots());

		findChild<QToolButton*>("toolButtonPlay")->setChecked(p->isPlaying());
		QSlider* hSliderTimeLine = findChild<QSlider*>("hSliderTimeLine");
		hSliderTimeLine->setMaximum(p->numFrames() - 1);
		hSliderTimeLine->setValue(p->frame());
		findChild<QLineEdit*>("lineEditFrameNumber")->setText(QString::number(p->frame() + 1));
		findChild<QLabel*>("labelFrameCount")->setText(QString::number(p->numFrames()));		

		targetPartModesChanged();
		updateProperties(part, p->modeName());
		connect(p, SIGNAL(selectNextMode()), this, SLOT(selectNextMode()));
		connect(p, SIGNAL(selectPreviousMode()), this, SLOT(selectPreviousMode()));
		connect(p, SIGNAL(frameChanged(int)), this, SLOT(frameChanged(int)));
		connect(p, SIGNAL(playActivated(bool)), this, SLOT(playActivated(bool)));

		this->setEnabled(true);
	}
	else {
		this->setEnabled(false);
	}
}

void AnimationWidget::targetPartNumFramesChanged() {
	if (mTarget) {
		QSlider* hSliderTimeLine = findChild<QSlider*>("hSliderTimeLine");
		hSliderTimeLine->setMaximum(mTarget->numFrames() - 1);
		hSliderTimeLine->setValue(mTarget->frame());
		findChild<QLineEdit*>("lineEditFrameNumber")->setText(QString::number(mTarget->frame() + 1));
		findChild<QLabel*>("labelFrameCount")->setText(QString::number(mTarget->numFrames()));
	}
}

void AnimationWidget::targetPartNumPivotsChanged() {
	// update number of pivots
	if (mTarget) {
		auto* numPivotsWidget = findChild<QSpinBox*>("spinBoxNumPivots");
		bool wha = numPivotsWidget->blockSignals(true);
		numPivotsWidget->setValue(mTarget->numPivots());
		numPivotsWidget->blockSignals(wha);
	}
}

void AnimationWidget::targetPartModesChanged() {
	if (mTarget) {
		mListWidgetModes->clear();
		mCurrentMode = mTarget->modeName();

		Part* part = PM()->getPart(mTarget->partRef());
		if (part) {
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

		targetPartNumFramesChanged();
		// mAnimatorWidget->targetPartModesChanged();
	}
}

void AnimationWidget::setNumPivots(int p) {
	if (mTarget) {
		TryCommand(new CChangeNumPivots(mTarget->partRef(), mTarget->modeName(), p));
	}
}

void AnimationWidget::modeActivated(const QString& mode) {
	if (mTarget) {
		mCurrentMode = mode;

		mListWidgetModes->blockSignals(true);
		auto items = mListWidgetModes->findItems(mode, Qt::MatchExactly);
		mListWidgetModes->clearSelection();
		for (auto* item : items) {
			mListWidgetModes->setItemSelected(item, true);
			mListWidgetModes->setCurrentItem(item);
		}
		mListWidgetModes->blockSignals(false);

		stop();
		mTarget->setMode(mode);

		Part* part = PM()->getPart(mTarget->partRef());
		updateProperties(part, mode);
		targetPartNumFramesChanged();
		targetPartNumPivotsChanged();
	}
}

void AnimationWidget::modeEditTextChanged(const QString& text) {
	if (mTarget && !mCurrentMode.isEmpty() && mCurrentMode != text && !text.isEmpty()) {
		stop();
		TryCommand(new CRenameMode(mTarget->partRef(), mCurrentMode, text));
	}
}

void AnimationWidget::addMode() {
	if (mTarget) {
		stop();
		TryCommand(new CNewMode(mTarget->partRef(), mCurrentMode));
	}
}

void AnimationWidget::copyMode() {
	if (mTarget) {
		stop();
		TryCommand(new CCopyMode(mTarget->partRef(), mCurrentMode));
	}
}

void AnimationWidget::deleteMode() {
	if (mTarget) {
		stop();
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

void AnimationWidget::resizeMode() {
	if (mTarget) {
		auto* part = PM()->getPart(mTarget->partRef());
		const auto& m = part->modes.value(mCurrentMode);

		stop();
		mResizeModeDialog->mLineEditWidth->setText(QString::number(m.width));
		mResizeModeDialog->mLineEditHeight->setText(QString::number(m.height));
		mResizeModeDialog->show();
	}
}

void AnimationWidget::resizeModeDialogAccepted() {
	if (mTarget) {
		int width = mResizeModeDialog->mLineEditWidth->text().toInt();
		int height = mResizeModeDialog->mLineEditHeight->text().toInt();
		int offsetx = mResizeModeDialog->mLineEditOffsetX->text().toInt();
		int offsety = mResizeModeDialog->mLineEditOffsetY->text().toInt();
		TryCommand(new CChangeModeSize(mTarget->partRef(), mCurrentMode, width, height, offsetx, offsety));
	}
}

void AnimationWidget::selectNextMode() {
	if (mListWidgetModes->count() > 1) {
		int row = mListWidgetModes->currentIndex().row();
		row = (row + 1) % mListWidgetModes->count();
		auto nextMode = mListWidgetModes->item(row)->text();
		modeActivated(nextMode);
	}
}

void AnimationWidget::selectPreviousMode() {
	if (mListWidgetModes->count() > 1) {
		int row = mListWidgetModes->currentIndex().row();
		row = (row + mListWidgetModes->count() - 1) % mListWidgetModes->count();
		auto nextMode = mListWidgetModes->item(row)->text();
		modeActivated(nextMode);
	}
}

void AnimationWidget::setFramerate(int frameRate) {
	TryCommand(new CChangeModeFPS(mTarget->partRef(), mTarget->modeName(), frameRate));
}

void AnimationWidget::frameChanged(int f) {
	if (mTarget) {
		findChild<QLineEdit*>("lineEditFrameNumber")->setText(QString::number(mTarget->frame() + 1));
		QSlider* sl = findChild<QSlider*>("hSliderTimeLine");
		bool wha = sl->blockSignals(true);
		sl->setValue(f);
		sl->blockSignals(wha);
	}
}

void AnimationWidget::addFrame() {
	if (mTarget) {
		stop();
		if (TryCommand(new CNewFrame(mTarget->partRef(), mTarget->modeName(), mTarget->numFrames()))) {
			goToFrame(mTarget->numFrames() - 1);
		}
	}
}

void AnimationWidget::copyFrame() {
	if (mTarget) {
		stop();
		if (TryCommand(new CCopyFrame(mTarget->partRef(), mTarget->modeName(), mTarget->frame()))) {
			goToFrame(mTarget->numFrames() - 1);
		}
	}
}

void AnimationWidget::deleteFrame() {
	if (mTarget) {
		stop();
		if (mTarget->numFrames() > 1) {
			TryCommand(new CDeleteFrame(mTarget->partRef(), mTarget->modeName(), mTarget->frame()));
		}
		else {
			// TODO: show error in status bar
		}

	}
}

void AnimationWidget::play(bool b) {
	if (mTarget) {
		mTarget->play(b);
	}
}

void AnimationWidget::playActivated(bool b) {
	findChild<QToolButton*>("toolButtonPlay")->setChecked(b);
}

void AnimationWidget::stop() {
	if (mTarget) {
		if (mTarget->isPlaying()) {
			mTarget->stop();
			findChild<QToolButton*>("toolButtonPlay")->setChecked(false);
		}
	}
}

void AnimationWidget::goToLastFrame() {
	if (mTarget) {
		goToFrame(mTarget->numFrames() - 1);
	}
}

void AnimationWidget::goToFirstFrame() {
	if (mTarget) {
		goToFrame(0);
	}
}

void AnimationWidget::goToNextFrame() {
	if (mTarget) {
		goToFrame((mTarget->frame() + 1) % mTarget->numFrames());
	}
}

void AnimationWidget::goToPrevFrame() {
	if (mTarget) {
		goToFrame((mTarget->frame() + mTarget->numFrames() - 1) % mTarget->numFrames());
	}
}

void AnimationWidget::goToFrame(int f) {
	if (mTarget) {
		if (mTarget->isPlaying()) {
			stop();
		}
		mTarget->setFrame(f);
	}
}


void AnimationWidget::updateProperties(Part* part, const QString& mode) {
	const auto& m = part->modes.value(mode);
	findChild<QLabel*>("labelModeSize")->setText(QString("%1x%2").arg(m.width).arg(m.height));
	findChild<QSpinBox*>("spinBoxFramerate")->setValue(m.framesPerSecond);
}
