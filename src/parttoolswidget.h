#ifndef PARTTOOLSWIDGET_H
#define PARTTOOLSWIDGET_H

#include "resizemodedialog.h"
#include <QWidget>
#include <QToolButton>
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QStandardItemModel>
#include <QTreeView>
#include <QDebug>
#include <QMimeData>

class AnimatorWidget;
class PartWidget;
class PaletteView;

namespace Ui {
class PartToolsWidget;
}

class PartToolsWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit PartToolsWidget(QWidget *parent = nullptr);
    ~PartToolsWidget();
    PartWidget* targetPartWidget();
    void setTargetPartWidget(PartWidget* p); // if p is null then no target widget
    
public slots:
    void chooseColour();
    void chooseColourByText(QString);
    void colourSelected(QColor);
    void textPropertiesEdited();
    void penChanged();
    void zoomChanged();
    void targetPartNumFramesChanged();
    void targetPartNumPivotsChanged();
    void targetPartModesChanged();
    void targetPartPropertiesChanged();

    void setToolTypeDraw();
    void setToolTypeErase();
    void setToolTypePickColour();
    void setToolTypeStamp();
    void setToolTypeCopy();
    void setToolTypeFill();

    void setNumPivots(int);

    void modeActivated(QString);

    void addMode();
    void copyMode();
    void deleteMode();
    void renameMode();
    void resizeMode();
    void resizeModeDialogAccepted();

    void addPalette();
    void deletePalette();
    void paletteActivated(QString str);

    void selectNextMode();
    void selectPreviousMode();

	void setFramerate(int);
private:
    Ui::PartToolsWidget *ui;
    PartWidget* mTarget;
    AnimatorWidget* mAnimatorWidget;

    QColor mPenColour;
    QToolButton* mToolButtonColour;
    QLineEdit* mLineEditColour;
    QPlainTextEdit* mTextEditProperties;
    QComboBox* mComboBoxModes;
    QPushButton* mPushButtonModeSize;    

    QComboBox* mComboBoxPalettes;
    PaletteView* mPaletteView;
    QStringList mDefaultPalettes;

    ResizeModeDialog* mResizeModeDialog;

    QAction *mActionDraw, *mActionErase, *mActionPickColour, *mActionStamp, *mActionCopy, *mActionFill;
};

#endif // PARTTOOLSWIDGET_H
