#include "mainwindow.h"

#include "ui_mainwindow.h"
#include "commands.h"
#include "partwidget.h"
#include "parttoolswidget.h"
#include "compositetoolswidget.h"

#include <QSortFilterProxyModel>
#include <QDebug>
#include <QMessageBox>
#include <QTextEdit>
#include <QToolButton>
#include <QtWidgets>
#include <QSettings>
#include <QDesktopServices>

#define APP_NAME (QString("mmpixel"))

static MainWindow* sWindow = nullptr;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mViewOptionsDockWidget(nullptr),
    mProjectModifiedSinceLastSave(false)
{
    sWindow = this;
    mProjectModel = new ProjectModel();
    // mProjectModel->loadTestData();

    ui->setupUi(this);

    QSettings settings;

    // Setup Part List widget
    // SETUP actions etc
    mPartList = findChild<PartList*>("partList");
    // mPartList->resize(1,1);
    connect(mPartList, SIGNAL(assetDoubleClicked(AssetRef)), this, SLOT(assetDoubleClicked(AssetRef)));

    // Set MDI Area
    mMdiArea = findChild<QMdiArea*>(tr("mdiArea"));
    // mMdiArea->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
    // mMdiArea->resize(100000,mMdiArea->height());
    connect(mMdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(subWindowActivated(QMdiSubWindow*)));

    // mMdiArea
    // QColor backgroundColour = QColor(settings.value("background_colour", QColor(111,198,143).rgba()).toUInt());
    QColor backgroundColour = QColor(settings.value("background_colour", QColor(255,255,255).rgba()).toUInt());
    mMdiArea->setBackground(QBrush(backgroundColour));

    QDockWidget* dockWidgetAssets = findChild<QDockWidget*>("dockWidgetAssets");
    dockWidgetAssets->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);

    mPartToolsWidget = new PartToolsWidget(this);
    mCompositeToolsWidget = new CompositeToolsWidget(this);

    mStackedWidget = new QStackedWidget(this);
    QLabel* label = new QLabel("No asset selected.", this);
    label->setAlignment(Qt::AlignCenter);
    mNoToolsIndex = mStackedWidget->addWidget(label);

    QHBoxLayout* partToolsHBox = new QHBoxLayout;
    partToolsHBox->addSpacerItem(new QSpacerItem(0,1,QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    partToolsHBox->addWidget(mPartToolsWidget);
    QWidget* partToolsHBoxWidget = new QWidget();
    partToolsHBoxWidget->setLayout(partToolsHBox);
    mPToolsIndex = mStackedWidget->addWidget(partToolsHBoxWidget);

    mCToolsIndex = mStackedWidget->addWidget(mCompositeToolsWidget);

    mStackedWidget->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    // mStackedWidget->resize(1,1);

    QDockWidget* dockWidgetTools = new QDockWidget("Tools", this);
    dockWidgetTools->setObjectName("dockWidgetTools");
    dockWidgetTools->setWidget(mStackedWidget);
    dockWidgetTools->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dockWidgetTools->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->addDockWidget(Qt::RightDockWidgetArea, dockWidgetTools);
    dockWidgetTools->show();
    dockWidgetTools->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    // dockWidgetTools->resize(1,1);


    // Remaining stuff..
    mUndoStack = new QUndoStack(this);
    createActions();
    createMenus();
    connect(mUndoStack, SIGNAL(indexChanged(int)), this, SLOT(undoStackIndexChanged(int)));

    // Final
    setWindowTitle(APP_NAME);
    restoreGeometry(settings.value("main_window_geometry").toByteArray());
    restoreState(settings.value("main_window_state").toByteArray());
}

MainWindow::~MainWindow()
{
    delete ui;
    delete mUndoStack;
    // delete ProjectModel last
    delete mProjectModel;
}

MainWindow* MainWindow::Instance(){
    return sWindow;
}

void MainWindow::createActions(){
    mUndoAction = mUndoStack->createUndoAction(this, tr("&Undo"));
    mUndoAction->setShortcuts(QKeySequence::Undo);

    mRedoAction = mUndoStack->createRedoAction(this, tr("&Redo"));
    mRedoAction->setShortcuts(QKeySequence::Redo);

    mAboutAction = new QAction(tr("About"), this);
    mAboutAction->setShortcut(QKeySequence::HelpContents);
    connect(mAboutAction, SIGNAL(triggered()), this, SLOT(showAbout()));

    mResetSettingsAction = new QAction(tr("Restore to Factory Settings"), this);
    connect(mResetSettingsAction, SIGNAL(triggered()), this, SLOT(resetSettings()));
}

void MainWindow::showViewOptionsDialog(){
    if (mViewOptionsDockWidget!=nullptr && mViewOptionsDockWidget->isHidden()){
        mViewOptionsDockWidget->show();
        return;
    }
    else if (mViewOptionsDockWidget!=nullptr){
        return;
    }

    mViewOptionsDockWidget = new QDockWidget(tr("View Preferences"), this);
    mViewOptionsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea |
                                            Qt::RightDockWidgetArea);
    mViewOptionsDockWidget->setWindowTitle("View Preferences");

    QWidget* centralWidget = new QFrame();
    QVBoxLayout* layout = new QVBoxLayout();
    centralWidget->setLayout(layout);
    mViewOptionsDockWidget->setWidget(centralWidget);

    // Options
    QSettings settings;

    {
        layout->addWidget(new QLabel("Background"));

        QPushButton* chgBGColour = new QPushButton("Change Colour");
        connect(chgBGColour, SIGNAL(clicked()), this, SLOT(changeBackgroundColour()));
        layout->addWidget(chgBGColour);
    }

    {
        layout->addWidget(new QLabel("Pivots"));

        QPushButton* changePivotColour = new QPushButton("Change Colour");
        connect(changePivotColour, SIGNAL(clicked()), this, SLOT(changePivotColour()));
        layout->addWidget(changePivotColour);

        QCheckBox* checkBox = new QCheckBox("Enabled");
        checkBox->setChecked(settings.value("pivot_enabled", true).toBool());
        connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(setPivotsEnabled(bool)));
        layout->addWidget(checkBox);

        QCheckBox* checkBox2 = new QCheckBox("Enabled During Playback");
        checkBox2->setChecked(settings.value("pivot_enabled_during_playback", true).toBool());
        connect(checkBox2, SIGNAL(toggled(bool)), this, SLOT(setPivotsEnabledDuringPlayback(bool)));
        layout->addWidget(checkBox2);
    }

    {
        layout->addWidget(new QLabel("Drop Shadow"));

        QSlider* dsoSlider = new QSlider(Qt::Horizontal);
        dsoSlider->setMinimum(0);
        dsoSlider->setMaximum(10);
        dsoSlider->setSingleStep(1);
        dsoSlider->setPageStep(2);
        dsoSlider->setValue(settings.value("drop_shadow_opacity", QVariant(0.f)).toFloat()*10);
        connect(dsoSlider, SIGNAL(valueChanged(int)), this, SLOT(setDropShadowOpacity(int)));
        QWidget* widget = new QWidget;
        widget->setLayout(new QHBoxLayout);
        widget->layout()->addWidget(new QLabel("Opacity"));
        widget->layout()->addWidget(dsoSlider);
        layout->addWidget(widget);

        QSlider* dsbSlider = new QSlider(Qt::Horizontal);
        dsbSlider->setMinimum(0);
        dsbSlider->setMaximum(10);
        dsbSlider->setSingleStep(1);
        dsbSlider->setPageStep(2);
        dsbSlider->setValue(settings.value("drop_shadow_blur", QVariant(0.5f)).toFloat()*10);
        connect(dsbSlider, SIGNAL(valueChanged(int)), this, SLOT(setDropShadowBlur(int)));
        widget = new QWidget;
        widget->setLayout(new QHBoxLayout);
        widget->layout()->addWidget(new QLabel("Blur"));
        widget->layout()->addWidget(dsbSlider);
        layout->addWidget(widget);

        QSlider* dsxSlider = new QSlider(Qt::Horizontal);
        dsxSlider->setMinimum(0);
        dsxSlider->setMaximum(10);
        dsxSlider->setSingleStep(1);
        dsxSlider->setPageStep(2);
        dsxSlider->setValue(settings.value("drop_shadow_offset_x", QVariant(0.5f)).toFloat()*10);
        connect(dsxSlider, SIGNAL(valueChanged(int)), this, SLOT(setDropShadowXOffset(int)));
        widget = new QWidget;
        widget->setLayout(new QHBoxLayout);
        widget->layout()->addWidget(new QLabel("X"));
        widget->layout()->addWidget(dsxSlider);
        layout->addWidget(widget);

        QSlider* dsySlider = new QSlider(Qt::Horizontal);
        dsySlider->setMinimum(0);
        dsySlider->setMaximum(10);
        dsySlider->setSingleStep(1);
        dsySlider->setPageStep(2);
        dsySlider->setValue(settings.value("drop_shadow_offset_y", QVariant(0.5f)).toFloat()*10);
        connect(dsySlider, SIGNAL(valueChanged(int)), this, SLOT(setDropShadowYOffset(int)));
        widget = new QWidget;
        widget->setLayout(new QHBoxLayout);
        widget->layout()->addWidget(new QLabel("Y"));
        widget->layout()->addWidget(dsySlider);
        layout->addWidget(widget);

        QPushButton* changeColour = new QPushButton("Change Colour");
        connect(changeColour, SIGNAL(clicked()), this, SLOT(changeDropShadowColour()));
        layout->addWidget(changeColour);
    }

    {
        layout->addWidget(new QLabel("Onion Skinning"));

        QSlider* osTransparencySlider = new QSlider(Qt::Horizontal);
        osTransparencySlider->setMinimum(0);
        osTransparencySlider->setMaximum(10);
        osTransparencySlider->setSingleStep(1);
        osTransparencySlider->setPageStep(2);
        osTransparencySlider->setValue(settings.value("onion_skinning_transparency", 0.0f).toFloat()*10);
        connect(osTransparencySlider, SIGNAL(valueChanged(int)), this, SLOT(setOnionSkinningTransparency(int)));
        QWidget* widget = new QWidget;
        widget->setLayout(new QHBoxLayout);
        widget->layout()->addWidget(new QLabel("Opacity"));
        widget->layout()->addWidget(osTransparencySlider);
        layout->addWidget(widget);

        QCheckBox* osCheckBox = new QCheckBox("Enabled During Playback");
        osCheckBox->setChecked(settings.value("onion_skinning_enabled_during_playback", false).toBool());
        connect(osCheckBox, SIGNAL(toggled(bool)), this, SLOT(setOnionSkinningEnabledDuringPlayback(bool)));
        layout->addWidget(osCheckBox);
    }

    layout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding));

    // Finally show the dockwidget

    this->addDockWidget(Qt::LeftDockWidgetArea, mViewOptionsDockWidget);
    mViewOptionsDockWidget->resize(0,250);
    mViewOptionsDockWidget->show();
}

void MainWindow::createMenus()
{
    ///////////////////////////
    // File
    ///////////////////////////

    mFileMenu = menuBar()->addMenu(tr("&File"));

    QAction* newProjectAction = mFileMenu->addAction("&New Project");
    newProjectAction->setShortcut(QKeySequence::New);
    connect(newProjectAction, SIGNAL(triggered()), this, SLOT(newProject()));

    QAction* loadProjectAction = mFileMenu->addAction("&Open Project");
    loadProjectAction->setShortcut(QKeySequence::Open);
    connect(loadProjectAction, SIGNAL(triggered()), this, SLOT(loadProject()));

    QAction* reloadProjectAction = mFileMenu->addAction("&Reopen Project");
    connect(reloadProjectAction, SIGNAL(triggered()), this, SLOT(reloadProject()));

    QAction* saveProjectAction = mFileMenu->addAction("Save Project");
    saveProjectAction->setShortcut(QKeySequence::Save);
    connect(saveProjectAction, SIGNAL(triggered()), this, SLOT(saveProject()));

    QAction* saveProjectActionAs = mFileMenu->addAction("&Save Project As..");
    saveProjectActionAs->setShortcut(QKeySequence::SaveAs);
    connect(saveProjectActionAs, SIGNAL(triggered()), this, SLOT(saveProjectAs()));

    QAction* quitAction = mFileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Close);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    ///////////////////////////
    // Edit
    ///////////////////////////

    mEditMenu = menuBar()->addMenu(tr("&Edit"));
    mEditMenu->addAction(mUndoAction);
    mEditMenu->addAction(mRedoAction);
    connect(mEditMenu->addAction("Show View Preferences"), SIGNAL(triggered()), this, SLOT(showViewOptionsDialog()));

    ///////////////////////////
    // About
    ///////////////////////////

    mHelpMenu = menuBar()->addMenu(tr("Help"));
    mHelpMenu->addAction(mAboutAction);
    mHelpMenu->addAction(mResetSettingsAction);
}

void MainWindow::showMessage(const QString& msg, int timeout){
    statusBar()->showMessage(msg, timeout);
}

void MainWindow::closeEvent(QCloseEvent*){
    // Save window layout..
    QSettings settings;
    settings.setValue("main_window_geometry", saveGeometry());
    settings.setValue("main_window_state", saveState());
}

void MainWindow::assetDoubleClicked(AssetRef ref){
    if (ref.type==AssetType::Part) openPartWidget(ref);
    else if (ref.type==AssetType::Composite) openCompositeWidget(ref);
}

void MainWindow::partWidgetClosed(PartWidget* pw){
    if (pw){
        mPartWidgets.remove(pw->partRef(), pw);
        subWindowActivated(nullptr);
        pw->deleteLater();
    }
}

void MainWindow::openPartWidget(AssetRef ref){
    if (PM()->hasPart(ref)){
        PartWidget* p = new PartWidget(ref, mMdiArea);
        mMdiArea->addSubWindow(p);
        p->show();
        connect(p, SIGNAL(closed(PartWidget*)), this, SLOT(partWidgetClosed(PartWidget*)));
        mPartWidgets.insertMulti(ref, p);
    }
}

void MainWindow::compositeWidgetClosed(CompositeWidget* cw){
    if (cw){
        mCompositeWidgets.remove(cw->compRef(), cw);
        subWindowActivated(nullptr);
        cw->deleteLater();
    }
}

void MainWindow::openCompositeWidget(AssetRef ref){
    if (PM()->hasComposite(ref)){
        CompositeWidget* p = new CompositeWidget(ref, mMdiArea);
        // p->setProperty("asset_type", AssetType::Composite);
        mMdiArea->addSubWindow(p);
        p->show();
        // NB: part widget
        connect(p, SIGNAL(closed(CompositeWidget*)), this, SLOT(compositeWidgetClosed(CompositeWidget*)));
        mCompositeWidgets.insertMulti(ref, p);
    }
}

void MainWindow::subWindowActivated(QMdiSubWindow* win){
    if (win==nullptr){
        mCompositeToolsWidget->setTargetCompWidget(nullptr);
        mPartToolsWidget->setTargetPartWidget(nullptr);

        mStackedWidget->setCurrentIndex(mNoToolsIndex);
    }
    else {
        if (dynamic_cast<PartWidget*>(win)){
            PartWidget* pw = (PartWidget*)(win);
            mPartToolsWidget->setTargetPartWidget(pw);
            mCompositeToolsWidget->setTargetCompWidget(nullptr);

            Part* part = PM()->getPart(pw->partRef());
            Q_ASSERT(part);
            mPartList->setSelection(part->ref);

            mStackedWidget->setCurrentIndex(mPToolsIndex);
        }
        else if (dynamic_cast<CompositeWidget*>(win)){
            CompositeWidget* cw = (CompositeWidget*)(win);
            mPartToolsWidget->setTargetPartWidget(nullptr);
            mCompositeToolsWidget->setTargetCompWidget(cw);

            Composite* comp = PM()->getComposite(cw->compRef());
            mPartList->setSelection(comp->ref);

            mStackedWidget->setCurrentIndex(mCToolsIndex);
        }
    }
}

PartWidget* MainWindow::activePartWidget(){
    QMdiSubWindow* win = mMdiArea->activeSubWindow();
    return dynamic_cast<PartWidget*>(win);
    //if (win->property("asset_type").toInt()==AssetType::Part)
    //    return (PartWidget*) win;
    //else return nullptr;
}

CompositeWidget* MainWindow::activeCompositeWidget(){
    QMdiSubWindow* win = mMdiArea->activeSubWindow();
    return dynamic_cast<CompositeWidget*>(win);

    /*
    if (win->property("asset_type").toInt()==AssetType::Composite)
        return (CompositeWidget*) win;
    else return nullptr;
    */
}

void MainWindow::partListChanged(){
    mPartList->updateList();

    // Delete any part widgets that don't exist anymore
    QMutableMapIterator<AssetRef,PartWidget*> i(mPartWidgets);
    while (i.hasNext()) {
        i.next();
        if (!PM()->hasPart(i.key())){
            // Close the widget
            i.value()->close();
        }
    }

    // Delete any comp widgets that don't exist anymore
    QMutableMapIterator<AssetRef,CompositeWidget*> i2(mCompositeWidgets);
    while (i2.hasNext()) {
        i2.next();
        if (!PM()->hasComposite(i2.key())){
            // Close the widget
            i2.value()->close();
        }
    }
}

void MainWindow::partRenamed(AssetRef ref, const QString& newName){
    mPartList->updateList();
    for(PartWidget* p: mPartWidgets.values(ref)){
        p->partNameChanged(newName);
    }

    /*
    while (mPartWidgets.contains(ref)){
        PartWidget* p = mPartWidgets.take(ref);
        p->partNameChanged(newName);
        mPartWidgets.insertMulti(newName, p);
    }
    */

    foreach(CompositeWidget* cw, mCompositeWidgets.values()){
        cw->partNameChanged(ref, newName);
    }

    if (mCompositeToolsWidget->isEnabled()){
        mCompositeToolsWidget->partNameChanged(ref, newName);
    }
}

void MainWindow::partFrameUpdated(AssetRef ref, const QString& mode, int frame){
    if (mPartWidgets.contains(ref)){
        foreach(PartWidget* p, mPartWidgets.values(ref)){
            p->partFrameUpdated(ref, mode, frame);
        }
    }

    foreach(CompositeWidget* cw, mCompositeWidgets.values()){
        cw->partFrameUpdated(ref, mode, frame);
    }
}

void MainWindow::partFramesUpdated(AssetRef ref, const QString& mode){
    foreach(PartWidget* p, mPartWidgets.values(ref)){
        p->partFramesUpdated(ref, mode);
    }

    if (mPartToolsWidget->isEnabled() && mPartToolsWidget->targetPartWidget()){
        if (mPartToolsWidget->targetPartWidget()->partRef()==ref && mPartToolsWidget->targetPartWidget()->modeName()==mode){
            mPartToolsWidget->targetPartNumFramesChanged();
        }
    }

    foreach(CompositeWidget* cw, mCompositeWidgets.values()){
        cw->partFramesUpdated(ref, mode);
    }
}

void MainWindow::partNumPivotsUpdated(AssetRef ref, const QString& mode){
    foreach(PartWidget* p, mPartWidgets.values(ref)){
        p->partNumPivotsUpdated(ref, mode);
    }

    if (mPartToolsWidget->targetPartWidget()){
        if (mPartToolsWidget->targetPartWidget()->partRef()==ref && mPartToolsWidget->targetPartWidget()->modeName()==mode){
            mPartToolsWidget->targetPartNumPivotsChanged();
        }
    }

    foreach(CompositeWidget* cw, mCompositeWidgets.values()){
        cw->partNumPivotsUpdated(ref, mode);
    }
}

void MainWindow::partPropertiesUpdated(AssetRef ref){
    foreach(PartWidget* p, mPartWidgets.values(ref)){
        p->partPropertiesChanged(ref);
    }

    if (mPartToolsWidget->targetPartWidget()){
        if (mPartToolsWidget->targetPartWidget()->partRef()==ref){
            mPartToolsWidget->targetPartPropertiesChanged();
        }
    }
}

void MainWindow::compPropertiesUpdated(AssetRef comp){
    foreach(CompositeWidget* cw, mCompositeWidgets.values(comp)){
        cw->updateCompFrames();
    }
    if (mCompositeToolsWidget->isEnabled()){
       if (mCompositeToolsWidget->compRef()==comp){
           mCompositeToolsWidget->targetCompPropertiesChanged();
        }
    }
}

void MainWindow::partModesChanged(AssetRef ref){
    const Part* part = PM()->getPart(ref);
    foreach(PartWidget* p, mPartWidgets.values(ref)){
        if (!part->modes.contains(p->modeName())){
            // mode has disappeared, so set a new mode..
            QString firstMode = *part->modes.keys().begin();
            p->setMode(firstMode);
        }
        else {
            p->updatePartFrames();
        }
    }

    if (mPartToolsWidget->targetPartWidget()){
        if (mPartToolsWidget->targetPartWidget()->partRef()==ref){
            mPartToolsWidget->targetPartModesChanged();
        }
    }

    // TODO: Tell composite widgets
    // foreach(CompositeWidget* cw, mCompositeWidgets.values()){
        // cw->partModesChanged(part);
    // }
    //if (mCompositeToolsWidget->isEnabled()){
    //    mCompositeToolsWidget->partModesChanged(ref);
    //}
}

void MainWindow::partModeRenamed(AssetRef ref, const QString& oldModeName, const QString& newModeName){
    foreach(PartWidget* p, mPartWidgets.values(ref)){
        if (p->modeName()==oldModeName){
           p->setMode(newModeName);
        }
    }

    if (mPartToolsWidget->targetPartWidget()){
        if (mPartToolsWidget->targetPartWidget()->partRef()==ref){
            mPartToolsWidget->targetPartModesChanged();
        }
    }

    // TODO: Tell composite widgets
}

void MainWindow::compositeRenamed(AssetRef ref, const QString& newName){
    mPartList->updateList();
    for(CompositeWidget* cw: mCompositeWidgets.values(ref)){
        cw->compNameChanged(ref);
    }

    if (mCompositeToolsWidget->isEnabled()){
        mCompositeToolsWidget->compNameChanged(ref, newName);
    }
}

void MainWindow::compositeUpdated(AssetRef ref){
    foreach(CompositeWidget* cw, mCompositeWidgets.values(ref)){
        cw->updateCompFrames();
    }

    if (mCompositeToolsWidget->isEnabled()){
         mCompositeToolsWidget->compositeUpdated(ref);
    }
}

void MainWindow::compositeUpdatedMinorChanges(AssetRef ref){
    foreach(CompositeWidget* cw, mCompositeWidgets.values(ref)){
        cw->updateCompFramesMinorChanges();
    }

    if (mCompositeToolsWidget->isEnabled()){
         mCompositeToolsWidget->compositeUpdated(ref);
    }
}

void MainWindow::folderRenamed(AssetRef ref, const QString& newName){
    mPartList->updateList();
    qDebug() << "TODO: Update the visual names/refs of parts and comps that are in this folder";
}


void MainWindow::changeBackgroundColour(){
    QSettings settings;
    // QColor backgroundColour = QColor(settings.value("background_colour", QColor(111,198,143).rgba()).toUInt());
    QColor backgroundColour = QColor(settings.value("background_colour", QColor(255,255,255).rgba()).toUInt());
    QColor col = QColorDialog::getColor(backgroundColour, this, tr("Select Background Colour"));
    if (col.isValid()){
        settings.setValue("background_colour", col.rgba());
        mMdiArea->setBackground(QBrush(col));

        // Update all view
        foreach (QWidget* w, this->mPartWidgets.values()){
            w->update();
        }
        foreach (QWidget* w, this->mCompositeWidgets.values()){
            w->update();
        }
    }
}

void MainWindow::changePivotColour(){
    QSettings settings;
    QColor colour = QColor(settings.value("pivot_colour", QColor(255,0,255).rgba()).toUInt());
    QColor col = QColorDialog::getColor(colour, this, tr("Select Pivot Colour"));
    if (col.isValid()){
        settings.setValue("pivot_colour", col.rgba());
        foreach (PartWidget* w, this->mPartWidgets.values()){
            w->updatePivots();
        }
        // TODO: tell compwidgets
    }
}

void MainWindow::setPivotsEnabled(bool enabled){
    QSettings settings;
    settings.setValue("pivot_enabled", enabled);
    foreach (PartWidget* pw, this->mPartWidgets.values()){
        pw->updatePivots();
    }
    // TODO: tell compwidgets
}

void MainWindow::setPivotsEnabledDuringPlayback(bool enabled){
    QSettings settings;
    settings.setValue("pivot_enabled_during_playback", enabled);
    foreach (PartWidget* pw, this->mPartWidgets.values()){
        pw->updatePivots();
    }
    // TODO: tell compwidgets
}

void MainWindow::setDropShadowOpacity(int d){
    QSettings settings;
    settings.setValue("drop_shadow_opacity", d/10.f);
    foreach (PartWidget* pw, this->mPartWidgets.values()){
        pw->updateDropShadow();
    }
    foreach (CompositeWidget* pw, this->mCompositeWidgets.values()){
        pw->updateDropShadow();
    }
}

void MainWindow::setDropShadowBlur(int val){
    QSettings settings;
    settings.setValue("drop_shadow_blur", val/10.f);
    foreach (PartWidget* pw, this->mPartWidgets.values()){
        pw->updateDropShadow();
    }
    foreach (CompositeWidget* pw, this->mCompositeWidgets.values()){
        pw->updateDropShadow();
    }
}

void MainWindow::setDropShadowXOffset(int val){
    QSettings settings;
    settings.setValue("drop_shadow_offset_x", val/10.f);
    foreach (PartWidget* pw, this->mPartWidgets.values()){
        pw->updateDropShadow();
    }
    foreach (CompositeWidget* pw, this->mCompositeWidgets.values()){
        pw->updateDropShadow();
    }
}

void MainWindow::setDropShadowYOffset(int val){
    QSettings settings;
    settings.setValue("drop_shadow_offset_y", val/10.f);
    foreach (PartWidget* pw, this->mPartWidgets.values()){
        pw->updateDropShadow();
    }
    foreach (CompositeWidget* pw, this->mCompositeWidgets.values()){
        pw->updateDropShadow();
    }
}

void MainWindow::changeDropShadowColour(){
    QSettings settings;
    QColor backgroundColour = QColor(settings.value("drop_shadow_colour", QColor(0,0,0).rgba()).toUInt());
    QColor col = QColorDialog::getColor(backgroundColour, this, tr("Select Drop Shadow Colour"));
    if (col.isValid()){
        settings.setValue("drop_shadow_colour", col.rgba());
        foreach (PartWidget* pw, this->mPartWidgets.values()){
            pw->updateDropShadow();
        }
        foreach (CompositeWidget* pw, this->mCompositeWidgets.values()){
            pw->updateDropShadow();
        }
    }
}

void MainWindow::setOnionSkinningTransparency(int val){
    QSettings settings;
    settings.setValue("onion_skinning_transparency", val/10.f);
    foreach (PartWidget* pw, this->mPartWidgets.values()){
        pw->updateOnionSkinning();
    }
}

void MainWindow::setOnionSkinningEnabledDuringPlayback(bool enabled){
    // qDebug() << "setOnionSkinningEnabledDuringPlayback(" << enabled << ")";
    QSettings settings;
    settings.setValue("onion_skinning_enabled_during_playback", enabled);
    foreach (PartWidget* pw, this->mPartWidgets.values()){
        pw->updateOnionSkinning();
    }

    // TODO: tell compwidgets
}

void MainWindow::showAbout(){
    QMessageBox::about(this, tr("mmpixel"), tr(
                           "<p>Help:<ul>"
                           "<li>Right-click (on colour): Select pen and pick colour</li>"
                           "<li>Right-click (on blank): Select eraser</li>"
                           "<li>Ctrl + Wheel: Zoom</li>"
                           "<li>Middle-press + drag: Move canvas</li>"
                           "<li>Space: Toggle playing in a part window</li>"
                           "<li>S/D: Change frame in part window</li>"
                           "<li>W/E: Change mode in part window</li>"
                           "<li>A: Place anchor in a part window</li>"
                           "<li>1-4: Place pivot in a part window</li>"
                           "</ul>"
                           "</p>"
                           "<p>mmpixel was created by <a href=\"http://twitter.com/eigenbom\">@eigenbom</a>."
                           "Thanks to <a href=\"http://adamwhitcroft.com/batch/\">@adamwhitcroft</a> for the UI icons and <a href=\"https://twitter.com/BlueSweatshirt\">@bluesweatshirt</a> for early discussions."
                           "The DB16 palette is by Dawnbringer.</p>"
                           ));
}

void MainWindow::resetSettings(){
    QMessageBox::StandardButton res = QMessageBox::question(this, "Restore to Factory Settings", "Restoring to factory settings will reset all your preferences. It will close the program down and you'll have to re-open it manually. Continue?");
    if (res==QMessageBox::Yes){
        MainWindow::Instance()->showMessage("Restoring Factory Settings");
        QSettings settings;
        settings.clear();
        QApplication::exit();
    }
}

void MainWindow::newProject(){
    QMessageBox::StandardButton button = QMessageBox::question(this, "Close Project?", "Really close the current project? All unsaved changes will be lost.");
    if (button==QMessageBox::Yes){
        // Close all windows and deactivate
        mCompositeToolsWidget->setTargetCompWidget(nullptr);
        mPartToolsWidget->setTargetPartWidget(nullptr);
        mMdiArea->closeAllSubWindows();

        // Clear undo stack
        mUndoStack->clear();

        // Clear current project model
        ProjectModel::Instance()->clear();
        mPartList->updateList();

        setWindowTitle(APP_NAME);

        MainWindow::Instance()->showMessage("New Project Started");
    }
}

void MainWindow::loadProject(const QString& fileName){
    QSettings settings;

    // Close all windows and deactivate
    mCompositeToolsWidget->setTargetCompWidget(nullptr);
    mPartToolsWidget->setTargetPartWidget(nullptr);
    mMdiArea->closeAllSubWindows();

    // Clear undo stack
    mUndoStack->clear();

    // Clear current project model
    ProjectModel::Instance()->clear();
    mPartList->updateList();

    // Try to load project
    bool result = ProjectModel::Instance()->load(fileName);
    if (!result){
        QMessageBox::warning(this, "Error during load", tr("Couldn't load project ") + fileName);

        ProjectModel::Instance()->clear();
        mPartList->updateList();
    }
    else {
        mPartList->updateList();

        // Update last loaded project location
        QDir lastOpenedPath = QFileInfo(fileName).absoluteDir();
        settings.setValue("last_load_dir", lastOpenedPath.absolutePath());

        // Reload all things that depend on QSettings..
        if (mViewOptionsDockWidget){
            mViewOptionsDockWidget->hide();
            delete mViewOptionsDockWidget;
            mViewOptionsDockWidget = nullptr;
        }

        QSettings settings;
        // QColor backgroundColour = QColor(settings.value("background_colour", QColor(111,198,143).rgba()).toUInt());
        QColor backgroundColour = QColor(settings.value("background_colour", QColor(255,255,255).rgba()).toUInt());
        mMdiArea->setBackground(QBrush(backgroundColour));

        mProjectModifiedSinceLastSave = false;
        setWindowTitle(APP_NAME + " [" + fileName + "]");

        MainWindow::Instance()->showMessage("Successfully loaded");
    }
}

void MainWindow::loadProject(){
    QSettings settings;
    QString dir = settings.value("last_load_dir", QDir::currentPath()).toString();

    QString fileName = QFileDialog::getOpenFileName(this, "Load Project", dir, "MoonPixel tar (*.tar)");
    if (fileName.isNull() || fileName.isEmpty() || !QFileInfo(fileName).isFile()){
        QMessageBox::warning(this, "Error during load", tr("Can't load project: ") + fileName);
    }
    else {
        loadProject(fileName);
    }
}

void MainWindow::reloadProject(){
    QString fileName = ProjectModel::Instance()->fileName;
    if (fileName.isNull() || fileName.isEmpty() || !QFileInfo(fileName).isFile()){
        QMessageBox::warning(this, "Error during reload", tr("Can't reload project: ") + fileName);
    }
    else {
        loadProject(fileName);
    }
}

void MainWindow::saveProject(){    
    QString fileName = ProjectModel::Instance()->fileName;

    if (fileName.isNull() || fileName.isEmpty() || !QFileInfo(fileName).isFile()){
        // Run Save As..
        saveProjectAs();
    }
    else {
        // Save to filename
        bool result = ProjectModel::Instance()->save(fileName);
        if (!result){
            QMessageBox::warning(this, "Error during save", tr("Couldn't save project as ") + fileName);
        }
        else {
            mProjectModifiedSinceLastSave = false;
            setWindowTitle(APP_NAME + " [" + fileName + "]");
            MainWindow::Instance()->showMessage("Successfully saved");
        }
    }
}

void MainWindow::saveProjectAs(){
    QSettings settings;
    QString dir = settings.value("last_save_dir", QDir::currentPath()).toString();

    QString fileName = QFileDialog::getSaveFileName(this, "Save Project As..", dir, "MoonPixel tar (*.tar)");
    if (!fileName.isNull()){
        bool result = ProjectModel::Instance()->save(fileName);
        if (!result){
            QMessageBox::warning(this, "Error during save", tr("Couldn't save project as ") + fileName);
        }
        else {
            // Update last saved project location
            QDir lastOpenedPath = QFileInfo(fileName).absoluteDir();
            settings.setValue("last_save_dir", lastOpenedPath.absolutePath());

            mProjectModifiedSinceLastSave = false;
            setWindowTitle(APP_NAME + " [" + fileName + "]");

            MainWindow::Instance()->showMessage("Successfully saved");
        }
    }
}

void MainWindow::undoStackIndexChanged(int){
    mProjectModifiedSinceLastSave = true;
    setWindowTitle(APP_NAME + " [" + PM()->fileName + "*]");
}

void MainWindow::openEigenbom(){
    QDesktopServices::openUrl(QUrl("http://twitter.com/eigenbom"));
}
