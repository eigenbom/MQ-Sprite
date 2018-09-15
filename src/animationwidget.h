#ifndef ANIMATIONWIDGET_H
#define ANIMATIONWIDGET_H

#include <QString>
#include <QWidget>

struct Part;
class PartWidget;
class ResizeModeDialog;
class ModeListWidget;

namespace Ui {
class AnimationWidget;
}

class AnimationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnimationWidget(QWidget *parent = nullptr);
    ~AnimationWidget();

	PartWidget* targetPartWidget();
	void setTargetPartWidget(PartWidget* p); // if p is null then no target widget

public slots:

	void targetPartNumFramesChanged();
	void targetPartNumPivotsChanged();
	void targetPartModesChanged();
	void setNumPivots(int);

	void modeActivated(const QString&);
	void modeEditTextChanged(const QString&);

	void addMode();
	void copyMode();
	void deleteMode();
	void resizeMode();
	void resizeModeDialogAccepted();

	void selectNextMode();
	void selectPreviousMode();

	void setFramerate(int);
	
	void frameChanged(int f);
	void addFrame();
	void copyFrame();
	void deleteFrame();

	void play(bool);
	void playActivated(bool);
	void stop();
	void goToLastFrame();
	void goToFirstFrame();
	void goToNextFrame();
	void goToPrevFrame();
	void goToFrame(int);

private:
	void updateProperties(Part* part, const QString& mode);

private:
    Ui::AnimationWidget *ui = nullptr;
	PartWidget* mTarget = nullptr;
	QString mCurrentMode {};
	ModeListWidget* mListWidgetModes = nullptr;
	ResizeModeDialog* mResizeModeDialog = nullptr;
};

#endif // ANIMATIONWIDGET_H
