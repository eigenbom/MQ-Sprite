#include "mainwindow.h"

#include "ui_mainwindow.h"
#include "commands.h"
#include "partwidget.h"
#include "compositetoolswidget.h"
#include "drawingtools.h"
#include "animationwidget.h"
#include "propertieswidget.h"
#include "optionswidget.h"

#include <QSortFilterProxyModel>
#include <QDebug>
#include <QMessageBox>
#include <QTextEdit>
#include <QToolButton>
#include <QtWidgets>
#include <QSettings>
#include <QDesktopServices>

static MainWindow* sWindow = nullptr;


static QString makeWindowTitle(QString filename = {}, bool saved = true) {
	static const QString appName { "MQ Sprite" };
	if (filename.isEmpty()) {
		filename = "untitled project";
		saved = false;
	}
	
	return filename + (saved ? "" : "*") + " - " + appName;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    sWindow = this;
    mProjectModel = new ProjectModel();	
    // mProjectModel->loadTestData();

    ui->setupUi(this);
	   	 
    

	// Load preferences
	loadPreferences();
	
    mPartList = findChild<PartList*>("partList");
    connect(mPartList, SIGNAL(assetDoubleClicked(AssetRef)), this, SLOT(assetDoubleClicked(AssetRef)));
	connect(mPartList, SIGNAL(assetSelected(AssetRef)), this, SLOT(assetSelected(AssetRef)));

	connect(mPartList->findChild<QToolButton*>("toolButtonNewSprite"), &QToolButton::clicked, [&]() { TryCommand(new CNewPart()); });
	connect(mPartList->findChild<QToolButton*>("toolButtonNewFolder"), &QToolButton::clicked, [&]() { TryCommand(new CNewFolder()); });
	connect(mPartList->findChild<QToolButton*>("toolButtonDeleteAsset"), &QToolButton::clicked, [&]() {
		if (mSelectedAsset.isNull()) {
			qWarning() << "Tried to delete asset but no asset is selected!";
		}
		else {
			switch (mSelectedAsset.type) {
			case AssetType::Part: TryCommand(new CDeletePart(mSelectedAsset)); break;
			case AssetType::Composite: TryCommand(new CDeleteComposite(mSelectedAsset)); break;
			case AssetType::Folder:TryCommand(new CDeleteFolder(mSelectedAsset)); break;
			}
			MainWindow::Instance()->partListChanged();
		}
	});

	connect(mPartList->findChild<QToolButton*>("toolButtonDuplicateAsset"), &QToolButton::clicked, [&]() {
		if (mSelectedAsset.isNull()) {
			qWarning() << "Tried to duplicate asset but no asset is selected!";
		}
		else {
			switch (mSelectedAsset.type) {
			case AssetType::Part: TryCommand(new CCopyPart(mSelectedAsset)); break;
			case AssetType::Composite: TryCommand(new CCopyComposite(mSelectedAsset)); break;
			case AssetType::Folder: qDebug() << "TODO: Copy folder command."; break;
			}
		}
	});
	
	// connect(findChild<QAction*>("actionNewSprite"), &QAction::triggered, [&]() { TryCommand(new CNewPart()); });
	// connect(findChild<QAction*>("actionNewFolder"), &QAction::triggered, [&]() { TryCommand(new CNewFolder()); });

    mMdiArea = findChild<QMdiArea*>(tr("mdiArea"));
    connect(mMdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(subWindowActivated(QMdiSubWindow*)));

    auto* assetsWidget = findChild<QFrame*>("assets");
	
    mUndoStack = new QUndoStack(this);
	connect(mUndoStack, SIGNAL(indexChanged(int)), this, SLOT(undoStackIndexChanged(int)));

	{
		auto* dock = new QDockWidget("Composite Tools", this);
		dock->setLayout(new QVBoxLayout());
		dock->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		mCompositeToolsWidget = new CompositeToolsWidget(this);
		dock->setWidget(mCompositeToolsWidget);
		dock->setAllowedAreas(Qt::DockWidgetArea::LeftDockWidgetArea | Qt::DockWidgetArea::RightDockWidgetArea);
		this->addDockWidget(Qt::RightDockWidgetArea, dock);
		dock->setFloating(true);
		dock->hide();
	}

	{
		auto* dock = new QDockWidget("Drawing Tools", this);
		dock->setLayout(new QVBoxLayout());
		dock->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		mDrawingTools = new DrawingTools(dock);
		dock->setWidget(mDrawingTools);
		dock->setAllowedAreas(Qt::DockWidgetArea::LeftDockWidgetArea | Qt::DockWidgetArea::RightDockWidgetArea);
		this->addDockWidget(Qt::RightDockWidgetArea, dock);
	}

	{
		auto* dock = new QDockWidget("Animations", this);
		dock->setLayout(new QVBoxLayout());
		dock->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		mAnimationWidget = new AnimationWidget(dock);
		dock->setWidget(mAnimationWidget);
		dock->setAllowedAreas(Qt::DockWidgetArea::LeftDockWidgetArea | Qt::DockWidgetArea::RightDockWidgetArea);
		this->addDockWidget(Qt::RightDockWidgetArea, dock);
	}

	{
		auto* dock = new QDockWidget("Properties", this);
		dock->setLayout(new QVBoxLayout());
		dock->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		// minSzHint = QSize(100, 100);
		mPropertiesWidget = new PropertiesWidget(dock);
		dock->setWidget(mPropertiesWidget);
		dock->setAllowedAreas(Qt::DockWidgetArea::LeftDockWidgetArea | Qt::DockWidgetArea::RightDockWidgetArea);
		this->addDockWidget(Qt::LeftDockWidgetArea, dock);
	}

    createActions();
    createMenus();
   
	{
		auto* action = dynamic_cast<QDockWidget*>(mDrawingTools->parentWidget())->toggleViewAction();
		action->setText("Drawing Tools Window");
		mViewMenu->addAction(action);
	}

	{
		auto* action = dynamic_cast<QDockWidget*>(mAnimationWidget->parentWidget())->toggleViewAction();
		action->setText("Animation Window");
		mViewMenu->addAction(action);
	}
	
	{
		auto* action = dynamic_cast<QDockWidget*>(mPropertiesWidget->parentWidget())->toggleViewAction();
		action->setText("Properties Window");
		mViewMenu->addAction(action);
	}

	{
		auto* action = dynamic_cast<QDockWidget*>(mCompositeToolsWidget->parentWidget())->toggleViewAction();
		action->setText("Composite Tools Window");
		mCompositeToolsWindowAction = action;

		// mViewMenu->addAction(action);
	}

	setWindowTitle(makeWindowTitle());

	{
		QSettings settings;
		restoreGeometry(settings.value("main_window_geometry").toByteArray());
		restoreState(settings.value("main_window_state").toByteArray());
	}
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
	mUndoAction->setIcon(QIcon(":/icon/icons/gentleface/undo_icon&16.png"));
	
    mRedoAction = mUndoStack->createRedoAction(this, tr("&Redo"));
    mRedoAction->setShortcuts(QKeySequence::Redo);
	mRedoAction->setIcon(QIcon(":/icon/icons/gentleface/redo_icon&16.png"));
		
	// findChild<QToolBar*>("toolBar")->hide();
	// findChild<QToolBar*>("toolBar")->addAction(mUndoAction);
	// findChild<QToolBar*>("toolBar")->addAction(mRedoAction);
	removeToolBar(findChild<QToolBar*>("toolBar"));

    mAboutAction = new QAction(tr("About"), this);
    mAboutAction->setShortcut(QKeySequence::HelpContents);
    connect(mAboutAction, SIGNAL(triggered()), this, SLOT(showAbout()));
}

void MainWindow::showViewOptionsDialog(){
    if (mViewOptionsDockWidget!=nullptr && mViewOptionsDockWidget->isHidden()){
        mViewOptionsDockWidget->show();
        return;
    }
    else if (mViewOptionsDockWidget!=nullptr){
        return;
    }

    mViewOptionsDockWidget = new QDockWidget(tr("Options"), this);
    mViewOptionsDockWidget->setWindowTitle("Options");

	auto* optionsWidget = new OptionsWidget(mViewOptionsDockWidget);
	mViewOptionsDockWidget->setWidget(optionsWidget);
	auto& prefs = GlobalPreferences();
	{
		auto* chgBGColour = optionsWidget->findChild<QPushButton*>("pushButtonBackgroundColour");
		connect(chgBGColour, SIGNAL(clicked()), this, SLOT(changeBackgroundColour()));

		auto* checkBox = optionsWidget->findChild<QCheckBox*>("checkBoxCheckerboard");
		checkBox->setChecked(prefs.backgroundCheckerboard);
		connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(setBackgroundGridPattern(bool)));
	    
        checkBox = optionsWidget->findChild<QCheckBox*>("checkBoxShowAnchors");
        checkBox->setChecked(prefs.showAnchors);
        connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(setPivotsEnabled(bool)));

        auto* spinBox = optionsWidget->findChild<QSpinBox*>("spinBoxMaxZoom");
        spinBox->setValue(prefs.maxZoom * 100);
        connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(changeMaxZoom(int)));
    }

	{
		auto* groupBox = optionsWidget->findChild<QGroupBox*>("groupBoxDropShadow");
		groupBox->setChecked(prefs.showDropShadow);
		connect(groupBox, &QGroupBox::toggled, [&, groupBox]() {
			GlobalPreferences().showDropShadow = groupBox->isChecked();
			this->updatePreferences();
		});

		QSlider* dsoSlider = optionsWidget->findChild<QSlider*>("horizontalSliderDropShadowOpacity");
		dsoSlider->setMinimum(0);
		dsoSlider->setMaximum(10);
		dsoSlider->setSingleStep(1);
		dsoSlider->setPageStep(2);
		dsoSlider->setValue(prefs.dropShadowOpacity * 10);
		connect(dsoSlider, SIGNAL(valueChanged(int)), this, SLOT(setDropShadowOpacity(int)));

		QSlider* dsbSlider = optionsWidget->findChild<QSlider*>("horizontalSliderDropShadowBlurSize");
		dsbSlider->setMinimum(0);
		dsbSlider->setMaximum(30);
		dsbSlider->setSingleStep(1);
		dsbSlider->setPageStep(2);
		dsbSlider->setValue(prefs.dropShadowBlurRadius * 10);
		connect(dsbSlider, SIGNAL(valueChanged(int)), this, SLOT(setDropShadowBlur(int)));

		QSlider* dsxSlider = optionsWidget->findChild<QSlider*>("horizontalSliderDropShadowH");
		dsxSlider->setMinimum(0);
		dsxSlider->setMaximum(10);
		dsxSlider->setSingleStep(1);
		dsxSlider->setPageStep(2);
		dsxSlider->setValue(prefs.dropShadowOffsetH * 10);
		connect(dsxSlider, SIGNAL(valueChanged(int)), this, SLOT(setDropShadowXOffset(int)));

		QSlider* dsySlider = optionsWidget->findChild<QSlider*>("horizontalSliderDropShadowV");
		dsySlider->setMinimum(0);
		dsySlider->setMaximum(10);
		dsySlider->setSingleStep(1);
		dsySlider->setPageStep(2);
		dsySlider->setValue(prefs.dropShadowOffsetV * 10);
		connect(dsySlider, SIGNAL(valueChanged(int)), this, SLOT(setDropShadowYOffset(int)));

		
		auto* changeColour = optionsWidget->findChild<QPushButton*>("pushButtonDropShadowColour");
		connect(changeColour, SIGNAL(clicked()), this, SLOT(changeDropShadowColour()));
	}

    {
		auto* groupBox = optionsWidget->findChild<QGroupBox*>("groupBoxOnionSkinning");
		groupBox->setChecked(prefs.showOnionSkinning);
		connect(groupBox, &QGroupBox::toggled, [&, groupBox]() {
			GlobalPreferences().showOnionSkinning = groupBox->isChecked();
			this->updatePreferences();
		});
		
		auto* osTransparencySlider = optionsWidget->findChild<QSlider*>("hSliderOnionSkinningOpacity");
        osTransparencySlider->setMinimum(0);
        osTransparencySlider->setMaximum(10);
        osTransparencySlider->setSingleStep(1);
        osTransparencySlider->setPageStep(2);
        osTransparencySlider->setValue(prefs.onionSkinningOpacity * 20);
        connect(osTransparencySlider, SIGNAL(valueChanged(int)), this, SLOT(setOnionSkinningTransparency(int)));       
    }

	{
		auto* resetButton = optionsWidget->findChild<QPushButton*>("pushButtonReset");
		connect(resetButton, SIGNAL(clicked()), this, SLOT(resetSettings()));
	}

    this->addDockWidget(Qt::NoDockWidgetArea, mViewOptionsDockWidget);
    mViewOptionsDockWidget->setFloating(true);
    // mViewOptionsDockWidget->resize(0,250);
    mViewOptionsDockWidget->show();
}

void MainWindow::createMenus(){
    mFileMenu = menuBar()->addMenu(tr("&File"));

    QAction* newProjectAction = mFileMenu->addAction("&New");
    newProjectAction->setShortcut(QKeySequence::New);
    connect(newProjectAction, SIGNAL(triggered()), this, SLOT(newProject()));

    QAction* loadProjectAction = mFileMenu->addAction("&Open...");
    loadProjectAction->setShortcut(QKeySequence::Open);
	connect(loadProjectAction, &QAction::triggered, this, [this]() {
		mMdiArea->closeAllSubWindows();
	});
    connect(loadProjectAction, SIGNAL(triggered()), this, SLOT(loadProject()));

	mFileMenu->addSeparator();

    QAction* reloadProjectAction = mFileMenu->addAction("Reopen");
	connect(reloadProjectAction, &QAction::triggered, this, [this]() {
		mMdiArea->closeAllSubWindows();
	});
    connect(reloadProjectAction, SIGNAL(triggered()), this, SLOT(reloadProject()));
		
	mFileMenu->addSeparator();

    QAction* saveProjectAction = mFileMenu->addAction("&Save");
    saveProjectAction->setShortcut(QKeySequence::Save);
    connect(saveProjectAction, SIGNAL(triggered()), this, SLOT(saveProject()));

    QAction* saveProjectActionAs = mFileMenu->addAction("Save As...");
    saveProjectActionAs->setShortcut(QKeySequence::SaveAs);
    connect(saveProjectActionAs, SIGNAL(triggered()), this, SLOT(saveProjectAs()));

	mFileMenu->addSeparator();

	QAction* exportProjectActionAs = mFileMenu->addAction("Export As...");	
	connect(exportProjectActionAs, SIGNAL(triggered()), this, SLOT(exportProjectAs()));

	mFileMenu->addSeparator();

    QAction* quitAction = mFileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Close);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    ///////////////////////////
    // Edit
    ///////////////////////////

    mEditMenu = menuBar()->addMenu(tr("&Edit"));
    mEditMenu->addAction(mUndoAction);
    mEditMenu->addAction(mRedoAction);
	
	/*
	mEditMenu->addSeparator();
	mDuplicateAssetAction = mEditMenu->addAction("&Duplicate");	
	mDuplicateAssetAction->setShortcut(QKeySequence(tr("Ctrl+D")));
	connect(mDuplicateAssetAction, &QAction::triggered, [&]() {
		if (mSelectedAsset.isNull()) {
			qWarning() << "Tried to duplicate asset, but no asset is selected!";
		}
		else {
			switch (mSelectedAsset.type) {
			case AssetType::Part: TryCommand(new CCopyPart(mSelectedAsset)); break;
			case AssetType::Composite: TryCommand(new CCopyComposite(mSelectedAsset)); break;
			case AssetType::Folder: qDebug() << "TODO: Copy folder"; break;
			}
		}
	});
	mDuplicateAssetAction->setEnabled(false);	
	*/

	mEditMenu->addSeparator();
    connect(mEditMenu->addAction("Options..."), SIGNAL(triggered()), this, SLOT(showViewOptionsDialog()));

	mViewMenu = menuBar()->addMenu(tr("&View"));
	mViewMenu->addSeparator();
	auto* action = mViewMenu->addAction("New Sprite View");
	mViewMenu->addSeparator();
	connect(action, &QAction::triggered, [&]() {
		auto* pw = this->activePartWidget();
		auto* cw = this->activeCompositeWidget();
		if (pw) this->openPartWidget(pw->partRef());
		if (cw) this->openCompositeWidget(cw->compRef());
	});
  
	mViewMenu->addSeparator();
	action = mViewMenu->addAction("Tabbed View");
	action->setCheckable(true);
	connect(action, &QAction::toggled, [&](bool checked) {
		this->ui->mdiArea->setViewMode(checked ? QMdiArea::ViewMode::TabbedView : QMdiArea::ViewMode::SubWindowView);
		GlobalPreferences().tabbedView = checked;
		this->updatePreferences();
	});
	action->setChecked(GlobalPreferences().tabbedView);
	mViewMenu->addSeparator();

	auto* spriteMenu = menuBar()->addMenu(tr("&Sprite"));
	mResizePartAction = spriteMenu->addAction("Resize...");
	connect(mResizePartAction, SIGNAL(triggered()), mAnimationWidget, SLOT(resizeMode()));
	mResizePartAction->setEnabled(false);

	
	auto toCamelCase = [](const QString& s) -> QString {
		QStringList parts = s.split(' ', QString::SkipEmptyParts);
		for (int i = 0; i < parts.size(); ++i)
			parts[i].replace(0, 1, parts[i][0].toUpper());

		return parts.join(" ");
	};

	{
		QSettings settings;
		auto theme = settings.value("theme", "windowsvista").toString();
		auto* styleMenu = menuBar()->addMenu(tr("Theme"));
		QActionGroup* ag = new QActionGroup(styleMenu);
		for (auto style : QStyleFactory::keys()) {
			auto* action  = ag->addAction(toCamelCase(style));
			action->setCheckable(true);
			if (style == theme) action->setChecked(true);
			connect(action, &QAction::triggered, [&, style, action]() {
				QApplication::setStyle(style);
				QSettings().setValue("theme", style);
			});
		}
		styleMenu->addActions(ag->actions());
	}

    mHelpMenu = menuBar()->addMenu(tr("Help"));
    mHelpMenu->addAction(mAboutAction);
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

void MainWindow::loadPreferences() {
	QSettings settings;

	auto& prefs = GlobalPreferences();
	prefs.backgroundColour = QColor(settings.value("prefs.backgroundColour", prefs.backgroundColour.name()).toString());
	prefs.backgroundCheckerboard = settings.value("prefs.backgroundCheckerboard", prefs.backgroundCheckerboard).toBool();
  prefs.showAnchors = settings.value("prefs.showAnchors", prefs.showAnchors).toBool();
  prefs.tabbedView = settings.value("prefs.tabbedView", prefs.tabbedView).toBool();
  prefs.maxZoom = settings.value("prefs.maxZoom", prefs.maxZoom).toInt();

	prefs.showDropShadow = settings.value("prefs.showDropShadow", prefs.showDropShadow).toBool();
	prefs.dropShadowColour = QColor(settings.value("prefs.dropShadowColour", prefs.dropShadowColour.name()).toString());
	prefs.dropShadowOpacity = settings.value("prefs.dropShadowOpacity", prefs.dropShadowOpacity).toFloat();
	prefs.dropShadowBlurRadius = settings.value("prefs.dropShadowBlurRadius", prefs.dropShadowBlurRadius).toFloat();
	prefs.dropShadowOffsetH = settings.value("prefs.dropShadowOffsetH", prefs.dropShadowOffsetH).toFloat();
	prefs.dropShadowOffsetV = settings.value("prefs.dropShadowOffsetV", prefs.dropShadowOffsetV).toFloat();

	prefs.showOnionSkinning = settings.value("prefs.showOnionSkinning", prefs.showOnionSkinning).toBool();
	prefs.onionSkinningOpacity = settings.value("prefs.onionSkinningOpacity", prefs.onionSkinningOpacity).toFloat();
}

void MainWindow::savePreferences() {
	const auto& prefs = GlobalPreferences();

	QSettings settings;

	settings.setValue("prefs.backgroundColour", prefs.backgroundColour.name());
	settings.setValue("prefs.backgroundCheckerboard", prefs.backgroundCheckerboard);
  settings.setValue("prefs.showAnchors", prefs.showAnchors);
  settings.setValue("prefs.tabbedView", prefs.tabbedView);
  settings.setValue("prefs.maxZoom", prefs.maxZoom);
	settings.setValue("prefs.showDropShadow", prefs.showDropShadow);
	settings.setValue("prefs.dropShadowColour", prefs.dropShadowColour.name());
	settings.setValue("prefs.dropShadowOpacity", prefs.dropShadowOpacity);
	settings.setValue("prefs.dropShadowBlurRadius", prefs.dropShadowBlurRadius);
	settings.setValue("prefs.dropShadowOffsetH", prefs.dropShadowOffsetH);
	settings.setValue("prefs.dropShadowOffsetV", prefs.dropShadowOffsetV);

	settings.setValue("prefs.showOnionSkinning", prefs.showOnionSkinning);
	settings.setValue("prefs.onionSkinningOpacity", prefs.onionSkinningOpacity);
}

void MainWindow::updatePreferences() {
	for (PartWidget* w : this->mPartWidgets.values()) {
		w->updateBackgroundBrushes();
		w->updatePartFrames();
		w->updateDropShadow();
		w->repaint();
	}
	for (CompositeWidget* w : this->mCompositeWidgets.values()) {
		w->updateBackgroundBrushes();
		w->updateCompFrames();
		w->updateDropShadow();
		w->repaint();
	}
	savePreferences();
}

void MainWindow::assetDoubleClicked(AssetRef ref){
	assetSelected(ref);

	switch (ref.type) {
	case AssetType::Part: {
		for (auto* win : mMdiArea->subWindowList()) {
			auto* pw = dynamic_cast<PartWidget*>(win);
			if (pw && pw->partRef() == ref) {
				pw->setFocus();
				return;
			}
		}
		openPartWidget(ref);
		break;
	}
	case AssetType::Composite: {
		for (auto* win : mMdiArea->subWindowList()) {
			auto* cw = dynamic_cast<CompositeWidget*>(win);
			if (cw && cw->compRef() == ref) {
				cw->setFocus();
				return;
			}
		}
		openCompositeWidget(ref);
		break;
	}
	}
}

void MainWindow::assetSelected(AssetRef ref) {
	if (mDuplicateAssetAction != nullptr) {
		if (ref.isNull()) {
			mDuplicateAssetAction->setEnabled(false);
		}
		else {
			switch (ref.type) {
			case AssetType::Folder: {
				break;
			}
			case AssetType::Part: {
				mDuplicateAssetAction->setEnabled(true);
				break;
			}
			case AssetType::Composite: {
				mDuplicateAssetAction->setEnabled(true);
				break;
			}
			}
		}
	}

	mSelectedAsset = ref;
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
		p->fitToWindow();
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

    /*
    if (mCompositeWidgets.size()==1 && mPartWidgets.size()==0){
        mMdiArea->tileSubWindows();
    }
    */

    // mMdiArea->tileSubWindows();
}

void MainWindow::subWindowActivated(QMdiSubWindow* win){
	mResizePartAction->setEnabled(false);
	
    if (win == nullptr){
        mCompositeToolsWidget->setTargetCompWidget(nullptr);
		mDrawingTools->setTargetPartWidget(nullptr);
		mAnimationWidget->setTargetPartWidget(nullptr);
		mPropertiesWidget->setTargetPartWidget(nullptr);

		mPartList->deselectAsset();
    }
    else {
		PartWidget* pw = dynamic_cast<PartWidget*>(win);
		CompositeWidget* cw = dynamic_cast<CompositeWidget*>(win);

        if (pw){
            mCompositeToolsWidget->setTargetCompWidget(nullptr);
			mDrawingTools->setTargetPartWidget(pw);
			mAnimationWidget->setTargetPartWidget(pw);
			mPropertiesWidget->setTargetPartWidget(pw);
			mResizePartAction->setEnabled(true);

			mPartList->selectAsset(pw->partRef());
        }
        else if (cw){
            mCompositeToolsWidget->setTargetCompWidget(cw);
			mDrawingTools->setTargetPartWidget(nullptr);
			mAnimationWidget->setTargetPartWidget(nullptr);
			mPropertiesWidget->setTargetPartWidget(nullptr);

			mPartList->selectAsset(cw->compRef());
        }
    }
}

PartWidget* MainWindow::activePartWidget(){
    QMdiSubWindow* win = mMdiArea->activeSubWindow();
    return dynamic_cast<PartWidget*>(win);
}

CompositeWidget* MainWindow::activeCompositeWidget(){
    QMdiSubWindow* win = mMdiArea->activeSubWindow();
    return dynamic_cast<CompositeWidget*>(win);
}

void MainWindow::partListChanged(){
    mPartList->updateList();

    // Delete any part widgets that don't exist anymore
    QMutableMapIterator<AssetRef,PartWidget*> i(mPartWidgets);
    while (i.hasNext()) {
        i.next();
        if (!PM()->hasPart(i.key())){
            i.value()->close();
        }
    }

    // Delete any comp widgets that don't exist anymore
    QMutableMapIterator<AssetRef,CompositeWidget*> i2(mCompositeWidgets);
    while (i2.hasNext()) {
        i2.next();
        if (!PM()->hasComposite(i2.key())){
            i2.value()->close();
        }
    }
}

void MainWindow::newAssetCreated(AssetRef ref) {
	mPartList->updateList();

	if (ref.type == AssetType::Part) {
		openPartWidget(ref);
		mPartList->selectAsset(ref);
	}
	else if (ref.type == AssetType::Composite) {
		openCompositeWidget(ref);
		mPartList->selectAsset(ref);
	}
	else {
		mPartList->selectAsset(ref);
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
        for(PartWidget* p: mPartWidgets.values(ref)){
            p->partFrameUpdated(ref, mode, frame);
        }
    }

    for(CompositeWidget* cw: mCompositeWidgets.values()){
        cw->partFrameUpdated(ref, mode, frame);
    }

	mPartList->updateIcon(ref);
}

void MainWindow::partFramesUpdated(AssetRef ref, const QString& mode){
    foreach(PartWidget* p, mPartWidgets.values(ref)){
        p->partFramesUpdated(ref, mode);
    }

	if (mAnimationWidget->isEnabled() && mAnimationWidget->targetPartWidget()) {
		if (mAnimationWidget->targetPartWidget()->partRef() == ref &&
			mAnimationWidget->targetPartWidget()->modeName() == mode) {
			mAnimationWidget->targetPartNumFramesChanged();
		}
	}

    foreach(CompositeWidget* cw, mCompositeWidgets.values()){
        cw->partFramesUpdated(ref, mode);
    }

	mPartList->updateIcon(ref);
}

void MainWindow::partNumPivotsUpdated(AssetRef ref, const QString& mode){
    foreach(PartWidget* p, mPartWidgets.values(ref)){
        p->partNumPivotsUpdated(ref, mode);
    }

	if (mAnimationWidget->targetPartWidget()) {
		if (mAnimationWidget->targetPartWidget()->partRef() == ref && mAnimationWidget->targetPartWidget()->modeName() == mode) {
			mAnimationWidget->targetPartNumPivotsChanged();
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

	if (mPropertiesWidget->targetPartWidget()) {
		if (mPropertiesWidget->targetPartWidget()->partRef() == ref) {
			mPropertiesWidget->targetPartPropertiesChanged();
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

	if (mAnimationWidget->targetPartWidget()) {
		if (mAnimationWidget->targetPartWidget()->partRef() == ref) {
			mAnimationWidget->targetPartModesChanged();
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

	if (mAnimationWidget->targetPartWidget()) {
		if (mAnimationWidget->targetPartWidget()->partRef() == ref) {
			mAnimationWidget->targetPartModesChanged();
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
    QColor col = QColorDialog::getColor(GlobalPreferences().backgroundColour, this, tr("Select Background Colour"));
    if (col.isValid()){
        GlobalPreferences().backgroundColour = col;
        updatePreferences();
    }
}

void MainWindow::changeMaxZoom(int zoom){
    GlobalPreferences().maxZoom = zoom / 100;
    updatePreferences();
}

void MainWindow::setBackgroundGridPattern(bool b){
	GlobalPreferences().backgroundCheckerboard = b;
	updatePreferences();
}

void MainWindow::setPivotsEnabled(bool enabled){
	GlobalPreferences().showAnchors = enabled;
	updatePreferences();
}

void MainWindow::setDropShadowOpacity(int sliderValue){
	GlobalPreferences().dropShadowOpacity = sliderValue / 10.0f;;
	updatePreferences();
}

void MainWindow::setDropShadowBlur(int sliderValue){
	GlobalPreferences().dropShadowBlurRadius = sliderValue / 10.f;
	updatePreferences();
}

void MainWindow::setDropShadowXOffset(int sliderValue){
	GlobalPreferences().dropShadowOffsetH = sliderValue / 10.f;
	updatePreferences();
}

void MainWindow::setDropShadowYOffset(int sliderValue){
	GlobalPreferences().dropShadowOffsetV = sliderValue / 10.f;
	updatePreferences();
}

void MainWindow::changeDropShadowColour(){
    QColor col = QColorDialog::getColor(GlobalPreferences().dropShadowColour, this, tr("Select Drop Shadow Colour"));
    if (col.isValid()){
		GlobalPreferences().dropShadowColour = col;
		updatePreferences();
    }
}

void MainWindow::setOnionSkinningTransparency(int sliderValue){
	GlobalPreferences().onionSkinningOpacity = sliderValue / 20.f;
	updatePreferences();
}

void MainWindow::showAbout(){
    QMessageBox::about(this, tr("About"), tr(
		"<h1>MQ Sprite</h1>"
"<p>Created by <a href=\"https://twitter.com/eigenbom\">Ben Porter</a> for <a href=\"https://playmoonquest.com\">MoonQuest</a>.</p>"
"<p>Uses <a href=\"http://www.gentleface.com/free_icon_set.html\">Gentleface Icons</a> (CC BY-NC 3.0).</p>"
"<p>Uses the \"Matriax8c\" palette by <a href=\"https://twitter.com/DavitMasia/\">Davit Masia</a>.</p>"
"<p>Shortcuts:<ul>"
	"<li>Right-click: Select colour (or eraser)</li>"
	"<li>Middle-click: Move canvas</li>"
	"<li>Mouse wheel: Zoom canvas</li>"
	"<li>Space: Toggle playback</li>"
	"<li>S, D: Change frame</li>"
	"<li>W, E: Change mode</li>"
	"<li>A, 1, 2, 3, 4: Move anchor or pivots</li>"
"</ul></p>"
     ));
}

void MainWindow::resetSettings(){
    QMessageBox::StandardButton res = QMessageBox::question(this, "Reset?", "This will clear your preferences and reset the program. The program will shut down. Continue?");
    if (res == QMessageBox::Yes){
        qInfo() << "Resetting preferences";
        QSettings settings;
        settings.clear();

		// TODO: Reload this window?
        QApplication::exit();
    }
}

void MainWindow::newProject(){
    QMessageBox::StandardButton button = QMessageBox::question(this, "Close Project?", "Really close the current project? All unsaved changes will be lost.");
    if (button==QMessageBox::Yes){
        // Close all windows and deactivate
        mCompositeToolsWidget->setTargetCompWidget(nullptr);
		mDrawingTools->setTargetPartWidget(nullptr);
		mAnimationWidget->setTargetPartWidget(nullptr);
		mPropertiesWidget->setTargetPartWidget(nullptr);
        mMdiArea->closeAllSubWindows();

        // Clear undo stack
        mUndoStack->clear();

        // Clear current project model
        ProjectModel::Instance()->clear();
		mPartList->resetIcons();
        mPartList->updateList();

        setWindowTitle(makeWindowTitle());
        qInfo() << "New Project";
    }
}

void MainWindow::loadProject(const QString& fileName){
    mCompositeToolsWidget->setTargetCompWidget(nullptr);
	mDrawingTools->setTargetPartWidget(nullptr);
	mAnimationWidget->setTargetPartWidget(nullptr);
	mPropertiesWidget->setTargetPartWidget(nullptr);

	// NB: This is done in a signal
    // mMdiArea->closeAllSubWindows();

    mUndoStack->clear();
    ProjectModel::Instance()->clear();
	mPartList->resetIcons();
    mPartList->updateList();
	
	/*
	QMessageBox* loadingMessage = new QMessageBox(this);
	loadingMessage->setMinimumWidth(400);
	loadingMessage->setWindowTitle("Loading project");
	loadingMessage->setText("Please wait...");
	loadingMessage->setStandardButtons(QMessageBox::NoButton);
	loadingMessage->setAttribute(Qt::WA_DeleteOnClose);
	loadingMessage->setModal(false);
	loadingMessage->open();
	*/
	
	QString reason;
    bool result = ProjectModel::Instance()->load(fileName, reason);

	// loadingMessage->done(QDialog::Accepted);
    if (!result){
		qDebug() << "Error while loading " << fileName << "! Reason: " << reason;
		QMessageBox::warning(this, "Error during load", tr("Couldn't load ") + fileName + tr("\nReason: ") + reason);
		if (!mProjectModel->importLog.isEmpty()) {
			QMessageBox::warning(this, "Import issues", mProjectModel->importLog.join("\n"));
			qDebug() << "Import issues:\n" << mProjectModel->importLog.join("\n");
		}
		mProjectModel->clear();
		mPartList->resetIcons();
        mPartList->updateList();
    }
    else {
		mPartList->resetIcons();
        mPartList->updateList();

        // Update last loaded project location
        QDir lastOpenedPath = QFileInfo(fileName).absoluteDir();
		QSettings settings;
        settings.setValue("last_save_dir", lastOpenedPath.absolutePath());

        // Reload all things that depend on QSettings..
        if (mViewOptionsDockWidget){
            mViewOptionsDockWidget->hide();
            delete mViewOptionsDockWidget;
            mViewOptionsDockWidget = nullptr;
        }

        mProjectModifiedSinceLastSave = false;
        setWindowTitle(makeWindowTitle(fileName, true));
        qInfo() << "Loaded " << fileName;

		if (PM()->composites.size() > 0 && !mViewMenu->actions().contains(mCompositeToolsWindowAction)) {
			// Only add composite tools window if loading a legacy project that contains composites
			mViewMenu->addAction(mCompositeToolsWindowAction);
		}

		if (!mProjectModel->importLog.isEmpty()) {
			QMessageBox::warning(this, "Import issues", mProjectModel->importLog.join("\n"));
		}
    }
}

void MainWindow::loadProject(){
    QSettings settings;
    QString dir = settings.value("last_save_dir", QDir::currentPath()).toString();

    QString fileName = QFileDialog::getOpenFileName(this, "Open...", dir, "MQ Sprite File (*.mqs)");
    if (fileName.isNull() || fileName.isEmpty() || !QFileInfo(fileName).isFile()){
        QMessageBox::warning(this, "Error during load", tr("Couldn't load ") + fileName);
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
        saveProjectAs();
    }
    else {
        bool result = ProjectModel::Instance()->save(fileName);
        if (!result){
			qWarning() << "Error during save";
			qWarning() << mProjectModel->exportLog.join("\n");
            QMessageBox::warning(this, "Error during save", tr("Couldn't save ") + fileName + "!\n" + mProjectModel->exportLog.mid(0, 10).join("\n"));
        }
        else {
            mProjectModifiedSinceLastSave = false;
			setWindowTitle(makeWindowTitle(fileName, true));
            MainWindow::Instance()->showMessage("Successfully saved");

			if (!mProjectModel->exportLog.isEmpty()) {
				qWarning() << "Error during save";
				qWarning() << mProjectModel->exportLog.join("\n");
				QMessageBox::warning(this, "Export issues", mProjectModel->exportLog.mid(0, 10).join("\n"));
			}
        }
    }
}

void MainWindow::saveProjectAs(){
    QSettings settings;
    QString dir = settings.value("last_save_dir", QDir::currentPath()).toString();

    QFileDialog saveDialog(this, "Save As...", dir, "MQ Sprite File (*.mqs)");
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix(".mqs");
    QString fileName;
    if (saveDialog.exec()) {
        fileName = saveDialog.selectedFiles().first();
    }
    if (!fileName.isNull()){
        bool result = ProjectModel::Instance()->save(fileName);
        if (!result){
			qWarning() << "Error during save";
			qWarning() << mProjectModel->exportLog.join("\n");
			QMessageBox::warning(this, "Error during save", tr("Couldn't save ") + fileName + "!\n" + mProjectModel->exportLog.mid(0, 10).join("\n"));
        }
        else {
            // Update last saved project location
            QDir lastOpenedPath = QFileInfo(fileName).absoluteDir();
            settings.setValue("last_save_dir", lastOpenedPath.absolutePath());

            mProjectModifiedSinceLastSave = false;
			setWindowTitle(makeWindowTitle(fileName, true));
            MainWindow::Instance()->showMessage("Successfully saved");

			if (!mProjectModel->exportLog.isEmpty()) {
				qWarning() << "Error during save";
				qWarning() << mProjectModel->exportLog.join("\n");
				QMessageBox::warning(this, "Export issues", mProjectModel->exportLog.mid(0, 10).join("\n"));
			}
        }
    }
}

void MainWindow::exportProjectAs() {
	QSettings settings;
	QString dir = settings.value("last_export_dir", QDir::currentPath()).toString();
	QString dirName = QFileDialog::getExistingDirectory(this, "Export To...", dir);
	if (!dirName.isNull()) {
		bool result = ProjectModel::Instance()->exportSimple(dirName);
		if (!result) {
			qWarning() << "Error during export";
			qWarning() << mProjectModel->exportLog.join("\n");
			QMessageBox::warning(this, "Error during export", tr("Couldn't export to ") + dirName + "!\n" + mProjectModel->exportLog.mid(0, 10).join("\n"));
		}
		else {
			settings.setValue("last_export_dir", QDir(dirName).absolutePath());
			MainWindow::Instance()->showMessage("Successfully exported");
			if (!mProjectModel->exportLog.isEmpty()) {
				qWarning() << "Error during export";
				qWarning() << mProjectModel->exportLog.join("\n");
				QMessageBox::warning(this, "Export issues", mProjectModel->exportLog.mid(0, 10).join("\n"));
			}
		}
	}
}

void MainWindow::undoStackIndexChanged(int){
    mProjectModifiedSinceLastSave = true;
	setWindowTitle(makeWindowTitle(PM()->fileName, false));
}
