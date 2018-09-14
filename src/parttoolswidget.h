#ifndef PARTTOOLSWIDGET_H
#define PARTTOOLSWIDGET_H

#include "resizemodedialog.h"
#include <QWidget>
#include <QColor>

struct Part;
class AnimatorWidget;
class PartWidget;
class ModeListWidget;

class QListWidget;
class QToolButton;
class QComboBox;
class QPushButton;
class QPlainTextEdit;
class QStandardItemModel;
class QTreeView;

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
	void modeEditTextChanged(const QString&);

    void addMode();
    void copyMode();
    void deleteMode();
    void resizeMode();
    void resizeModeDialogAccepted();

    void selectNextMode();
    void selectPreviousMode();

	void setFramerate(int);

private:
	void updateProperties(Part* part, const QString& mode);

private:
    Ui::PartToolsWidget *ui;
    PartWidget* mTarget;
	QString mCurrentMode;
    AnimatorWidget* mAnimatorWidget;

    QColor mPenColour;
    QToolButton* mToolButtonColour;
    QPlainTextEdit* mTextEditProperties; 
	ModeListWidget* mListWidgetModes;	
	
    QComboBox* mComboBoxPalettes;
    QStringList mDefaultPalettes;

    ResizeModeDialog* mResizeModeDialog;

	QAction* mActionDraw;
	QAction* mActionErase; 
	QAction* mActionPickColour; 
	QAction* mActionStamp; 
	QAction* mActionCopy; 
	QAction* mActionFill;
};

#endif // PARTTOOLSWIDGET_H
