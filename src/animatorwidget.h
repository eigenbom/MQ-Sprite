#ifndef ANIMATORWIDGET_H
#define ANIMATORWIDGET_H

#include <QFrame>
#include <QWidget>
#include <QToolButton>
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>

class PartWidget;

namespace Ui {
class AnimatorWidget;
}

class AnimatorWidget : public QFrame
{
    Q_OBJECT

public:
    explicit AnimatorWidget(QWidget *parent = nullptr);
    ~AnimatorWidget();

    PartWidget* targetPartWidget();
    void setTargetPartWidget(PartWidget* p); // if p is null then no target widget

public slots:
    void frameChanged(int f);
    void targetPartNumFramesChanged();
    void targetPartModesChanged();
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

    void modeActivated(QString);
    void setModeFPS();
    void setPlaybackSpeedMultiplier(int);

private:
    Ui::AnimatorWidget *ui;
    PartWidget* mTarget;

    // QSlider* mHSliderPlaybackSpeedMultiplier;
    // QLineEdit* mLineEditPlaybackSpeedMultiplier;
    QPushButton* mPushButtonModeFPS;
};

#endif // ANIMATORWIDGET_H
