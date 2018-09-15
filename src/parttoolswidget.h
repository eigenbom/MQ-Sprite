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
    void textPropertiesEdited();
    void targetPartNumFramesChanged();
    void targetPartNumPivotsChanged();
    void targetPartModesChanged();
    void targetPartPropertiesChanged();
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
    QPlainTextEdit* mTextEditProperties; 
	ModeListWidget* mListWidgetModes;    
	ResizeModeDialog* mResizeModeDialog;
};

#endif // PARTTOOLSWIDGET_H
