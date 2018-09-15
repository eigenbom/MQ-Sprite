#ifndef DRAWINGTOOLS_H
#define DRAWINGTOOLS_H

#include <QColor>
#include <QWidget>

class QAction;
class QToolButton;

class PartWidget;

namespace Ui {
class DrawingTools;
}

class DrawingTools : public QWidget
{
    Q_OBJECT

public:
    explicit DrawingTools(QWidget *parent = nullptr);
    ~DrawingTools();

	PartWidget* targetPartWidget();
	void setTargetPartWidget(PartWidget* p);

public slots:
	void showColourDialog();
	void penChanged();
	void zoomChanged();

private:
	void setColourIcon(QToolButton*, QColor);
	void selectColour(QColor);

private:
	// Widgets
    Ui::DrawingTools *ui = nullptr;
	QToolButton* mToolButtonColour = nullptr;
	QAction* mActionDraw = nullptr;
	QAction* mActionErase = nullptr;
	QAction* mActionPickColour = nullptr;
	QAction* mActionStamp = nullptr;
	QAction* mActionCopy = nullptr;
	QAction* mActionFill = nullptr;

	// Internal state
	PartWidget* mTarget = nullptr;
	QColor mPenColour {};
};

#endif // DRAWINGTOOLS_H
