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

class CompositeToolsWidget;
class DrawingTools;
class PropertiesWidget;
class AnimationWidget;

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
	void newAssetCreated(AssetRef ref);

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
	void loadPreferences();
	void savePreferences();
	void updatePreferences();

public slots:
    void assetDoubleClicked(AssetRef ref);
	void assetSelected(AssetRef ref);

    void partWidgetClosed(PartWidget*);
    void openPartWidget(AssetRef ref);
    void compositeWidgetClosed(CompositeWidget*);
    void openCompositeWidget(AssetRef ref);
    void subWindowActivated(QMdiSubWindow*);
    void showViewOptionsDialog();

    // Misc actions
    void changeBackgroundColour();
    void setBackgroundGridPattern(bool);
    void setPivotsEnabled(bool);
    void setDropShadowOpacity(int);
    void setDropShadowBlur(int);
    void setDropShadowXOffset(int);
    void setDropShadowYOffset(int);
    void changeDropShadowColour();
    void setOnionSkinningTransparency(int);

    void showAbout();
    void resetSettings();

    void newProject();
    void loadProject(const QString& fileName);
    void loadProject();
    void reloadProject();
    void saveProject();
    void saveProjectAs();

    void undoStackIndexChanged(int);

private:
    Ui::MainWindow *ui = nullptr;
    QMdiArea* mMdiArea = nullptr;
    ProjectModel* mProjectModel = nullptr;
    PartList* mPartList = nullptr;
    CompositeToolsWidget* mCompositeToolsWidget = nullptr;
	DrawingTools* mDrawingTools = nullptr;
	PropertiesWidget* mPropertiesWidget = nullptr;
	AnimationWidget* mAnimationWidget = nullptr;
    QDockWidget *mViewOptionsDockWidget = nullptr;
    QMultiMap<AssetRef,PartWidget*> mPartWidgets;
    QMultiMap<AssetRef,CompositeWidget*> mCompositeWidgets;

	AssetRef mSelectedAsset {};

    QUndoStack* mUndoStack = nullptr;
    QMenu *mFileMenu = nullptr;
    QMenu *mEditMenu = nullptr;
    QMenu *mViewMenu = nullptr;
    QMenu *mHelpMenu = nullptr;

    QAction* mUndoAction = nullptr;
    QAction* mRedoAction = nullptr;
    QAction* mAboutAction = nullptr;

	QAction* mResizePartAction = nullptr;
	QAction* mDuplicateAssetAction = nullptr;

    bool mProjectModifiedSinceLastSave = false;
};

#endif // MAINWINDOW_H
