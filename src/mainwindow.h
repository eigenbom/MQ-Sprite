#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QListView>
#include <QUndoStack>
#include <QMdiArea>
#include <QStackedWidget>

#include "projectmodel.h"
#include "partwidget.h"
#include "compositewidget.h"
#include "partlist.h"

class PartToolsWidget;
class CompositeToolsWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    static MainWindow* Instance();
    QUndoStack* undoStack(){return mUndoStack;}

    void createActions();
    void createMenus();
    void showMessage(const QString& msg, int timeout=2000);

    // Notifications from commands that something has changed in the project
    void partListChanged();
    void partRenamed(AssetRef ref, const QString& newName);
    void partFrameUpdated(AssetRef ref, const QString& mode, int frame);
    void partFramesUpdated(AssetRef ref, const QString& mode);
    void partNumPivotsUpdated(AssetRef ref, const QString& mode);
    void partPropertiesUpdated(AssetRef ref);
    void partModesChanged(AssetRef ref);
    void partModeRenamed(AssetRef ref, const QString& oldModeName, const QString& newModeName);

    void compositeRenamed(AssetRef ref, const QString& newName);
    void compositeUpdated(AssetRef ref);
    void compositeUpdatedMinorChanges(AssetRef ref);
    void compPropertiesUpdated(AssetRef ref);

    void folderRenamed(AssetRef ref, const QString& newName);

    PartWidget* activePartWidget();
    CompositeWidget* activeCompositeWidget();

protected:
    void closeEvent(QCloseEvent* event);

public slots:
    void assetDoubleClicked(AssetRef ref);
    void partWidgetClosed(PartWidget*);
    void openPartWidget(AssetRef ref);
    void compositeWidgetClosed(CompositeWidget*);
    void openCompositeWidget(AssetRef ref);
    void subWindowActivated(QMdiSubWindow*);
    void showViewOptionsDialog();

    // Misc actions
    void changeBackgroundColour();
    void setBackgroundGridPattern(bool);
    void changePivotColour();
    void setPivotsEnabled(bool);
    void setPivotsEnabledDuringPlayback(bool);
    void setDropShadowOpacity(int);
    void setDropShadowBlur(int);
    void setDropShadowXOffset(int);
    void setDropShadowYOffset(int);
    void changeDropShadowColour();
    void setOnionSkinningTransparency(int);
    void setOnionSkinningEnabledDuringPlayback(bool);

    void showAbout();
    void resetSettings();

    void newProject();
    void loadProject(const QString& fileName);
    void loadProject();
    void reloadProject();
    void saveProject();
    void saveProjectAs();

    void undoStackIndexChanged(int);
    void openEigenbom();

private:
    Ui::MainWindow *ui;
    QMdiArea* mMdiArea;
    ProjectModel* mProjectModel;
    PartList* mPartList;
    PartToolsWidget* mPartToolsWidget;
    CompositeToolsWidget* mCompositeToolsWidget;
    QDockWidget *mViewOptionsDockWidget;

    QStackedWidget *mStackedWidget;
    int mNoToolsIndex, mPToolsIndex, mCToolsIndex;

    QMultiMap<AssetRef,PartWidget*> mPartWidgets;
    QMultiMap<AssetRef,CompositeWidget*> mCompositeWidgets;

    QUndoStack* mUndoStack;

    QMenu *mFileMenu;
    QMenu *mEditMenu;
    QMenu *mViewMenu;
    QMenu *mHelpMenu;

    QAction* mUndoAction;
    QAction* mRedoAction;
    QAction* mAboutAction;

	QAction* mResizePartAction;

    bool mProjectModifiedSinceLastSave;
};

#endif // MAINWINDOW_H
