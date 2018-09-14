#include "drawingtools.h"
#include "ui_drawingtools.h"

#include "partwidget.h"
#include <array>
#include <utility>
#include <QActionGroup>
#include <QColorDialog>

DrawingTools::DrawingTools(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DrawingTools),
	mPenColour(QColor("#101820"))
{
    ui->setupUi(this);

	mToolButtonColour = findChild<QToolButton*>("toolButtonColour");
	connect(mToolButtonColour, SIGNAL(clicked()), this, SLOT(showColourDialog()));
	setColourIcon(mToolButtonColour, mPenColour);

	std::array<std::pair<const char*, const char*>, 8> colours = { {
		{ "colourDefault_1", "#101820" },
		{ "colourDefault_2", "#736464" },
		{ "colourDefault_3", "#a0694b" },
		{ "colourDefault_4", "#d24040" },
		{ "colourDefault_5", "#00a0c8" },
		{ "colourDefault_6", "#10c840" },
		{ "colourDefault_7", "#fac800" },
		{ "colourDefault_8", "#f0f0dc" },
	} };

	for (auto cp: colours){
		QColor colour = QColor(cp.second);
		auto* tb = findChild<QToolButton*>(cp.first);
		if (tb) {
			setColourIcon(tb, colour);
			connect(tb, &QToolButton::clicked, [this, colour]() {
				selectColour(colour);
			});
		}
	}


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

	auto setDrawToolTypeAction = [this](DrawToolType type) {
		return [this, type]() {
			if (mTarget) mTarget->setDrawToolType(type);
		};
	};

	connect(mActionDraw, &QAction::triggered, setDrawToolTypeAction(kDrawToolPaint));
	connect(mActionErase, &QAction::triggered, setDrawToolTypeAction(kDrawToolEraser));
	connect(mActionPickColour, &QAction::triggered, setDrawToolTypeAction(kDrawToolPickColour));
	connect(mActionStamp, &QAction::triggered, setDrawToolTypeAction(kDrawToolStamp));
	connect(mActionCopy, &QAction::triggered, setDrawToolTypeAction(kDrawToolCopy));
	connect(mActionFill, &QAction::triggered, setDrawToolTypeAction(kDrawToolFill));

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

	// Disable until a part becomes active
	setEnabled(false);
}

DrawingTools::~DrawingTools()
{
    delete ui;
}

PartWidget* DrawingTools::targetPartWidget() {
	return mTarget;
}

void DrawingTools::setTargetPartWidget(PartWidget* p) {
	// Disconnect and connect signals
	if (mTarget) {
		disconnect(findChild<QSlider*>("hSliderPenSize"), SIGNAL(valueChanged(int)), mTarget, SLOT(setPenSize(int)));
		// disconnect(findChild<QSlider*>("hSliderZoom"), SIGNAL(valueChanged(int)), mTarget, SLOT(setZoom(int)));
		// disconnect(findChild<QToolButton*>("toolButtonFitToWindow"), SIGNAL(clicked()), mTarget, SLOT(fitToWindow()));
		// disconnect(mTarget, SIGNAL(zoomChanged()), this, SLOT(zoomChanged()));
		disconnect(mTarget, SIGNAL(penChanged()), this, SLOT(penChanged()));
		mTarget = nullptr;
	}
	if (p) {
		mTarget = p;
		// findChild<QSlider*>("hSliderZoom")->setValue(p->zoom());
		p->setPenSize(findChild<QSlider*>("hSliderPenSize")->value());
		p->setPenColour(mPenColour);
		for (auto* action : mActionDraw->actionGroup()->actions()) {
			if (action->isChecked()) action->trigger();
		}
		//connect(findChild<QSlider*>("hSliderZoom"), SIGNAL(valueChanged(int)), p, SLOT(setZoom(int)));
		//connect(findChild<QToolButton*>("toolButtonFitToWindow"), SIGNAL(clicked()), p, SLOT(fitToWindow()));
		connect(findChild<QSlider*>("hSliderPenSize"), SIGNAL(valueChanged(int)), p, SLOT(setPenSize(int)));
		// connect(p, SIGNAL(zoomChanged()), this, SLOT(zoomChanged()));
		connect(p, SIGNAL(penChanged()), this, SLOT(penChanged()));
		this->setEnabled(true);
	}
	else {
		this->setEnabled(false);
	}
}

void DrawingTools::showColourDialog() {
	selectColour(QColorDialog::getColor(mPenColour));
}

void DrawingTools::penChanged() {
	if (mTarget) {
		findChild<QSlider*>("hSliderPenSize")->setValue(mTarget->penSize());
		findChild<QLabel*>("labelPenSize")->setText(QString("%1 px").arg(mTarget->penSize()));
		mPenColour = mTarget->penColour();
		setColourIcon(mToolButtonColour, mPenColour);
		switch (mTarget->drawToolType()) {
		case kDrawToolPaint: mActionDraw->trigger(); break;
		case kDrawToolEraser: mActionErase->trigger(); break;
		case kDrawToolPickColour: mActionPickColour->trigger(); break;
		default: break;
		}
	}
}

void DrawingTools::zoomChanged() {
	if (mTarget) {
		findChild<QSlider*>("hSliderZoom")->setValue(mTarget->zoom());
	}
}

void DrawingTools::setColourIcon(QToolButton* toolButton, QColor colour)
{
	QPixmap px(toolButton->iconSize());
	px.fill(colour);
	toolButton->setIcon(px);
	toolButton->setToolTip(colour.name());
}

void DrawingTools::selectColour(QColor colour) {
	mPenColour = colour;
	setColourIcon(mToolButtonColour, mPenColour);
	if (mTarget) {
		mTarget->setPenColour(mPenColour);
		if (mTarget->drawToolType() != kDrawToolPaint && mTarget->drawToolType() != kDrawToolFill) {
			mTarget->setDrawToolType(kDrawToolPaint);
		}
		penChanged();
	}
}