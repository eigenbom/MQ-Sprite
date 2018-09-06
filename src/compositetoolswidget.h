#ifndef COMPOSITETOOLSWIDGET_H
#define COMPOSITETOOLSWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QListWidget>
#include <QVBoxLayout>
#include <QSignalMapper>
#include <QPlainTextEdit>
#include "projectmodel.h"

class CompositeWidget;

namespace Ui {
class CompositeToolsWidget;
}

class CompositeToolsWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit CompositeToolsWidget(QWidget *parent = nullptr);
    ~CompositeToolsWidget();

    void setTargetCompWidget(CompositeWidget*);
    void updateTable();
    void updateSet();

    // Updates
    void partNameChanged(AssetRef part, const QString& newPartName);
    void compNameChanged(AssetRef comp, const QString& newCompName);
    void compositeUpdated(AssetRef comp);

    AssetRef compRef() const;
    void targetCompPropertiesChanged();

public slots:
    void addPart();
    void deletePart();
    void cellChanged(int,int);
    void play(bool);
    void playActivated(bool);
    void textPropertiesEdited();

    void modeSelected(const QString&);
    void loopToggled(const QString&);
    void visibleToggled(const QString&);

    void setPlaybackSpeedMultiplier(int);
    
private:
    Ui::CompositeToolsWidget *ui;
    CompositeWidget* mTarget;
    QTableWidget* mTableWidgetParts;
    QVBoxLayout* mSetLayout;
    QWidget* mWidgetSet;
    QSlider* mHSliderPlaybackSpeedMultiplier;
    QLineEdit* mLineEditPlaybackSpeedMultiplier;
    QPlainTextEdit* mTextEditProperties;

    QMap<QString, QComboBox*> mChildModeComboBox;
    QMap<QString, QCheckBox*> mChildLoopCheckBox;
    QMap<QString, QCheckBox*> mChildVisibleCheckBox;

    QSignalMapper* mModesSignalMapper;
    QSignalMapper* mLoopSignalMapper;
    QSignalMapper* mVisibleSignalMapper;
};

#endif // COMPOSITETOOLSWIDGET_H
