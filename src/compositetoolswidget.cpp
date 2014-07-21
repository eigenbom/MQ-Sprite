#include "compositetoolswidget.h"
#include "ui_compositetoolswidget.h"
#include "compositewidget.h"
#include "projectmodel.h"
#include "commands.h"
#include "mainwindow.h"

#include <QDebug>
#include <QUndoStack>
#include <QLabel>
#include <QScrollArea>
#include <QSignalMapper>

static const int NUM_PLAYBACK_SPEED_MULTIPLIERS = 7;
static float PLAYBACK_SPEED_MULTIPLIERS[NUM_PLAYBACK_SPEED_MULTIPLIERS] = {1./8,1./4,1./2,1,2,4,8};
static const char* PLAYBACK_SPEED_MULTIPLIER_LABELS[NUM_PLAYBACK_SPEED_MULTIPLIERS] = {"1/8","1/4","1/2","1","2","4","8"};

CompositeToolsWidget::CompositeToolsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CompositeToolsWidget),
    mTarget(nullptr),
    mModesSignalMapper(nullptr),
    mLoopSignalMapper(nullptr),
    mVisibleSignalMapper(nullptr)
{
    ui->setupUi(this);

    mTableWidgetParts = findChild<QTableWidget*>("tableWidgetParts");
    connect(mTableWidgetParts, SIGNAL(cellChanged(int,int)), this, SLOT(cellChanged(int,int)));
    connect(findChild<QToolButton*>("toolButtonAddPart"), SIGNAL(clicked()), this, SLOT(addPart()));
    connect(findChild<QToolButton*>("toolButtonDeletePart"), SIGNAL(clicked()), this, SLOT(deletePart()));
    connect(findChild<QToolButton*>("toolButtonPlaySet"), SIGNAL(clicked(bool)), this, SLOT(play(bool)));

    mHSliderPlaybackSpeedMultiplier = findChild<QSlider*>("hSliderPlaybackSpeedMultiplier");
    mLineEditPlaybackSpeedMultiplier = findChild<QLineEdit*>("lineEditPlaybackSpeedMultiplier");
    connect(mHSliderPlaybackSpeedMultiplier, SIGNAL(valueChanged(int)), this, SLOT(setPlaybackSpeedMultiplier(int)));

    mTextEditProperties = findChild<QPlainTextEdit*>("textEditProperties");
    connect(mTextEditProperties, SIGNAL(textChanged()), this, SLOT(textPropertiesEdited()));

    mWidgetSet = findChild<QScrollArea*>("scrollAreaSet");
    QWidget *widget = new QWidget();
    mSetLayout = new QVBoxLayout(widget);
    ((QScrollArea*)mWidgetSet)->setWidget(widget);
}

CompositeToolsWidget::~CompositeToolsWidget()
{
    delete ui;
}

void CompositeToolsWidget::setTargetCompWidget(CompositeWidget* cw){
    if (mTarget){
        // Clear
        mTableWidgetParts->clear();
        while (mTableWidgetParts->rowCount()>0){
            mTableWidgetParts->removeRow(0);
        }

        QLayoutItem* item;
        while ( ( item = mSetLayout->takeAt( 0 ) ) != nullptr )
        {
            delete item->widget();
            delete item;
        }

        mChildLoopCheckBox.clear();
        mChildModeComboBox.clear();

        // disconnect
        disconnect(mTarget, SIGNAL(playActivated(bool)), this, SLOT(playActivated(bool)));

        mTarget = nullptr;
    }

    if (cw){
        mTarget = cw;

        updateTable();
        updateSet();

        // set properties
        mTextEditProperties->blockSignals(true);
        mTextEditProperties->setPlainText(cw->properties());
        mTextEditProperties->blockSignals(false);

        // connect signals
        connect(mTarget, SIGNAL(playActivated(bool)), this, SLOT(playActivated(bool)));

        // Finally enable it
        this->setEnabled(true);
    }
    else {
        this->setEnabled(false);
    }
}

void CompositeToolsWidget::updateTable(){
    Q_ASSERT(mTarget);

    // Setup UI with proper values
    mTableWidgetParts->clear();
    while (mTableWidgetParts->rowCount()>0){
        mTableWidgetParts->removeRow(0);
    }
    QStringList headerLabels;
    headerLabels << "name" << "part name" << "z" << "p" << "piv";
    mTableWidgetParts->setHorizontalHeaderLabels(headerLabels);
    // mTableWidgetParts->horizontalHeader()->resizeSection(0, 0);
    mTableWidgetParts->verticalHeader()->setDefaultSectionSize(16);

    mTableWidgetParts->blockSignals(true);
    Composite* comp = PM()->getComposite(mTarget->compRef());
    if (comp){
        int root = comp->root;
        int index = 0;
        foreach(QString childName, comp->children){
            Q_ASSERT(comp->childrenMap.contains(childName));
            const Composite::Child& ch = comp->childrenMap.value(childName);

            // ch.part
            int row = mTableWidgetParts->rowCount();
            mTableWidgetParts->insertRow(row);

            Part* part = PM()->getPart(ch.part);
            QString partName = part?part->name:QString("");
            mTableWidgetParts->setItem(row,0, new QTableWidgetItem(childName));
            mTableWidgetParts->setItem(row,1, new QTableWidgetItem(partName));
            mTableWidgetParts->setItem(row,2, new QTableWidgetItem(QString::number(ch.z)));
            mTableWidgetParts->setItem(row,3, new QTableWidgetItem(QString::number(ch.parent)));
            mTableWidgetParts->setItem(row,4, new QTableWidgetItem(QString::number(ch.parentPivot)));
            mTableWidgetParts->setVerticalHeaderItem(row, new QTableWidgetItem(QString::number(row)));


            if (index==root){
                QFont font = mTableWidgetParts->item(row,0)->font();
                font.setBold(true);
                mTableWidgetParts->item(row,0)->setFont(font);
            }

            index++;
        }
    }

    mTableWidgetParts->setVisible(false);
    mTableWidgetParts->resizeColumnsToContents();
    mTableWidgetParts->setVisible(true);
    mTableWidgetParts->blockSignals(false);
}

void CompositeToolsWidget::updateSet(){
    Q_ASSERT(mTarget);

    // Setup UI with proper values
    QLayoutItem* item;
    while ( ( item = mSetLayout->takeAt( 0 ) ) != nullptr )
    {
        delete item->widget();
        delete item;
    }
    mChildLoopCheckBox.clear();
    mChildModeComboBox.clear();
    mChildVisibleCheckBox.clear();;
    delete mModesSignalMapper;
    delete mLoopSignalMapper;
    delete mVisibleSignalMapper;
    mModesSignalMapper = nullptr;
    mLoopSignalMapper = nullptr;
    mVisibleSignalMapper = nullptr;

    // Create set UI
    Composite* comp = PM()->getComposite(mTarget->compRef());
    if (comp){
        // int root = comp->root;
        // int index = 0;

        mModesSignalMapper = new QSignalMapper(this);
        mLoopSignalMapper = new QSignalMapper(this);
        mVisibleSignalMapper = new QSignalMapper(this);

        foreach(QString childName, comp->children){
            Q_ASSERT(comp->childrenMap.contains(childName));
            const Composite::Child& ch = comp->childrenMap.value(childName);

            QWidget* widget = new QWidget;
            QHBoxLayout* layout = new QHBoxLayout();
            layout->setContentsMargins(0,0,0,0);

            layout->addWidget(new QLabel(childName));
            layout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::MinimumExpanding));
            QComboBox* comboBoxModes = new QComboBox();
            comboBoxModes->setToolTip("Change mode");
            if (PM()->hasPart(ch.part)){
                const Part* part = PM()->getPart(ch.part);
                foreach(const QString& mode, part->modes.keys()){
                    comboBoxModes->addItem(mode);
                }
            }
            layout->addWidget(comboBoxModes);
            mChildModeComboBox.insert(childName, comboBoxModes);

            connect(comboBoxModes, SIGNAL(currentIndexChanged(int)), mModesSignalMapper, SLOT(map()));
            mModesSignalMapper->setMapping(comboBoxModes, childName);

            QCheckBox* checkBoxLoop = new QCheckBox();
            checkBoxLoop->setToolTip("Toggle looping");
            mChildLoopCheckBox.insert(childName, checkBoxLoop);
            layout->addWidget(checkBoxLoop);
            connect(checkBoxLoop, SIGNAL(toggled(bool)), mLoopSignalMapper, SLOT(map()));
            mLoopSignalMapper->setMapping(checkBoxLoop, childName);

            QCheckBox* checkBoxVisible = new QCheckBox();
            checkBoxVisible->setToolTip("Toggle visibility");
            mChildVisibleCheckBox.insert(childName, checkBoxVisible);
            layout->addWidget(checkBoxVisible);
            connect(checkBoxVisible, SIGNAL(toggled(bool)), mVisibleSignalMapper, SLOT(map()));
            mVisibleSignalMapper->setMapping(checkBoxVisible, childName);

            widget->setLayout(layout);
            mSetLayout->addWidget(widget);

            QString currentMode = mTarget->modeForCurrentSet(childName);
            bool currentLoop = mTarget->loopForCurrentSet(childName);
            bool currentVisible = mTarget->visibleForCurrentSet(childName);

            comboBoxModes->blockSignals(true);
            comboBoxModes->setCurrentIndex(comboBoxModes->findText(currentMode));
            comboBoxModes->blockSignals(false);

            checkBoxLoop->blockSignals(true);
            checkBoxLoop->setChecked(currentLoop);
            checkBoxLoop->blockSignals(false);

            checkBoxVisible->blockSignals(true);
            checkBoxVisible->setChecked(currentVisible);
            checkBoxVisible->blockSignals(false);
        }

        connect(mModesSignalMapper, SIGNAL(mapped(const QString &)),this, SLOT(modeSelected(const QString &)));
        connect(mLoopSignalMapper, SIGNAL(mapped(const QString &)),this, SLOT(loopToggled(const QString &)));
        connect(mVisibleSignalMapper, SIGNAL(mapped(const QString &)),this, SLOT(visibleToggled(const QString &)));

        mSetLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding));
    }

    // set play button toggled if playing..
    findChild<QToolButton*>("toolButtonPlaySet")->setChecked(mTarget->isPlaying());

    int psmi = mTarget->playbackSpeedMultiplierIndex();
    if (psmi==-1){
        psmi = 3;
        mTarget->setPlaybackSpeedMultiplier(psmi, PLAYBACK_SPEED_MULTIPLIERS[psmi]);
    }
    mHSliderPlaybackSpeedMultiplier->setValue(psmi);
}

void CompositeToolsWidget::partNameChanged(AssetRef part, const QString& newPartName){
    // Update part names in comp
    if (mTarget){
        Composite* comp = PM()->getComposite(mTarget->compRef());
        for(int i=0;i<mTableWidgetParts->rowCount();i++){
            const QString& childName = comp->children[i];
            const Composite::Child& child = comp->childrenMap.value(childName);
            if (child.part==part){
                mTableWidgetParts->setItem(i,1,new QTableWidgetItem(newPartName));
            }

            // const QString& str = mTableWidgetParts->item(i,1)->text();
            // if (str==oldPartName){
            //                mTableWidgetParts->setItem(i,1,new QTableWidgetItem(newPartName));
            //            }
        }
    }
}

void CompositeToolsWidget::compNameChanged(AssetRef ref, const QString& /*newCompName*/){
    if (mTarget && mTarget->compRef()==ref){
        // TODO ?
    }
}

void CompositeToolsWidget::compositeUpdated(AssetRef ref){
    if (mTarget && mTarget->compRef()==ref){
        updateTable();
        updateSet();
    }
}

AssetRef CompositeToolsWidget::compRef() const {
    return mTarget?mTarget->compRef():AssetRef();
}

void CompositeToolsWidget::targetCompPropertiesChanged(){
    if (mTarget){
        Composite* comp = PM()->getComposite(mTarget->compRef());
        if (comp){
            int cursorPos = mTextEditProperties->textCursor().position();
            mTextEditProperties->blockSignals(true);
            mTextEditProperties->setPlainText(comp->properties);
            mTextEditProperties->blockSignals(false);

            QTextCursor cursor = mTextEditProperties->textCursor();
            cursor.setPosition(cursorPos, QTextCursor::MoveAnchor);
            mTextEditProperties->setTextCursor(cursor);
        }
    }
}

void CompositeToolsWidget::textPropertiesEdited(){
    if (mTarget){
        ChangeCompPropertiesCommand* command = new ChangeCompPropertiesCommand(mTarget->compRef(), mTextEditProperties->toPlainText());
        if (command->ok){
            MainWindow::Instance()->undoStack()->push(command);
        }
        else {
            delete command;
        }
    }
}

void CompositeToolsWidget::addPart(){
    if (mTarget){
        if (mTarget->isPlaying()) mTarget->play(false);

        // Add a new child command
        NewCompositeChildCommand* command = new NewCompositeChildCommand(mTarget->compRef());
        if (command->ok){
            MainWindow::Instance()->undoStack()->push(command);
        }
        else {
            delete command;
        }
    }
}

void CompositeToolsWidget::deletePart(){
    if (mTarget){
        if (mTarget->isPlaying()) mTarget->play(false);

        // Delete the selected row (if any)
        int row = mTableWidgetParts->currentRow();
        const Composite* comp = PM()->getComposite(mTarget->compRef());
        if (comp){
            if (row>=0 && row<comp->children.size()){
                const QString& childName = comp->children.at(row);
                DeleteCompositeChildCommand* command = new DeleteCompositeChildCommand(mTarget->compRef(), childName);
                if (command->ok){
                    MainWindow::Instance()->undoStack()->push(command);
                }
                else {
                    delete command;
                }
            }
        }
    }
}

void CompositeToolsWidget::cellChanged(int row, int /*column*/){
    if (mTarget){
        const Composite* comp = PM()->getComposite(mTarget->compRef());
        const QString& childName = mTableWidgetParts->item(row, 0)->text();
        const QString& partName = mTableWidgetParts->item(row, 1)->text();
        bool ok = false;
        int z = mTableWidgetParts->item(row, 2)->text().toInt(&ok);
        if (!ok) z = 0;
        int parent = mTableWidgetParts->item(row, 3)->text().toInt(&ok);
        if (!ok) parent = -1;
        int parentPivot = mTableWidgetParts->item(row, 4)->text().toInt(&ok);
        if (!ok) parentPivot = -1;
        if (comp->children.at(row)!=childName){
            EditCompositeChildNameCommand* command = new EditCompositeChildNameCommand(mTarget->compRef(), comp->children.at(row), childName);
            if (command->ok){
                MainWindow::Instance()->undoStack()->push(command);
            }
            else {
                delete command;
            }
        }
        else {
            const Composite::Child& child = comp->childrenMap.value(childName);
            Part* part = PM()->getPart(child.part);
            QString partName = part?part->name:QString();
            if (partName!=partName || child.parent!=parent || child.parentPivot!=parentPivot || child.z!=z){
                // Update..
                EditCompositeChildCommand* command = new EditCompositeChildCommand(mTarget->compRef(), childName, partName, z, parent, parentPivot);
                if (command->ok){
                    MainWindow::Instance()->undoStack()->push(command);
                }
                else {
                    // TODO: error?
                    delete command;
                }
            }
        }
    }
}

void CompositeToolsWidget::play(bool b){
    if (mTarget){
        mTarget->play(b);
    }
}

void CompositeToolsWidget::playActivated(bool b){
    findChild<QToolButton*>("toolButtonPlaySet")->setChecked(b);
}

void CompositeToolsWidget::modeSelected(const QString& child){
    QComboBox* cb = mChildModeComboBox.value(child);
    if (cb){
        mTarget->setChildMode(child, cb->currentText());
    }
}

void CompositeToolsWidget::loopToggled(const QString& child){
    QCheckBox* cb = mChildLoopCheckBox.value(child);
    if (cb){
        mTarget->setChildLoop(child, cb->checkState()==Qt::Checked);
    }
}

void CompositeToolsWidget::visibleToggled(const QString& child){
    QCheckBox* cb = mChildVisibleCheckBox.value(child);
    if (cb){
        mTarget->setChildVisible(child, cb->checkState()==Qt::Checked);
    }
}

void CompositeToolsWidget::setPlaybackSpeedMultiplier(int i){
    Q_ASSERT(i>=0 && i<NUM_PLAYBACK_SPEED_MULTIPLIERS);
    if (mTarget){
        float playbackSpeed = PLAYBACK_SPEED_MULTIPLIERS[i];
        const char* label = PLAYBACK_SPEED_MULTIPLIER_LABELS[i];
        mLineEditPlaybackSpeedMultiplier->setText(tr("x") + label);
        mTarget->setPlaybackSpeedMultiplier(i, playbackSpeed);
    }
}

