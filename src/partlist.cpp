#include "partlist.h"
#include "ui_partlist.h"

#include "mainwindow.h"
#include "commands.h"

#include <QtWidgets>

PartList::PartList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PartList)
{
    ui->setupUi(this);
    mAssetListWidget = findChild<AssetListWidget*>("assetList");
    connect(mAssetListWidget, SIGNAL(assetDoubleClicked(QString,int)), this, SIGNAL(assetDoubleClicked(QString,int)));
    connect(findChild<QToolButton*>("toolButtonNewPart"), SIGNAL(clicked()), this, SLOT(newPart()));
    connect(findChild<QToolButton*>("toolButtonNewComp"), SIGNAL(clicked()), this, SLOT(newComp()));
    connect(findChild<QToolButton*>("toolButtonRenameAsset"), SIGNAL(clicked()), this, SLOT(renameAsset()));
    connect(findChild<QToolButton*>("toolButtonCopyAsset"), SIGNAL(clicked()), this, SLOT(copyAsset()));
    connect(findChild<QToolButton*>("toolButtonDeleteAsset"), SIGNAL(clicked()), this, SLOT(deleteAsset()));
    updateList();
}

PartList::~PartList()
{
    delete ui;
}

void PartList::newPart()
{
    NewPartCommand* command = new NewPartCommand();
    if (command->ok) MainWindow::Instance()->undoStack()->push(command);
    else delete command;
}

void PartList::newComp()
{
    NewCompositeCommand* command = new NewCompositeCommand();
    if (command->ok) MainWindow::Instance()->undoStack()->push(command);
    else delete command;
}

void PartList::renameAsset()
{
    QListWidgetItem* item = mAssetListWidget->currentItem();
    if (item){
        int id = item->data(Qt::UserRole).toInt();
        const QString& name = mAssetListWidget->assetName(id);
        int type = mAssetListWidget->assetType(id);
        bool ok;
        QString text = QInputDialog::getText(this, tr("Rename Asset"),
                                             tr("Rename ") + name + ":", QLineEdit::Normal,
                                             name, &ok);
        if (ok && !text.isEmpty()){
            if (type==ASSET_TYPE_PART){
                RenamePartCommand* command = new RenamePartCommand(name, text);
                if (command->ok){
                    MainWindow::Instance()->undoStack()->push(command);
                }
                else {
                    delete command;
                }
            }
            else if (type==ASSET_TYPE_COMPOSITE){
                RenameCompositeCommand* command = new RenameCompositeCommand(name, text);
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

void PartList::copyAsset()
{
    QListWidgetItem* item = mAssetListWidget->currentItem();
    if (item){
        int id = item->data(Qt::UserRole).toInt();
        const QString& name = mAssetListWidget->assetName(id);
        int type = mAssetListWidget->assetType(id);
        if (type==ASSET_TYPE_PART){
            CopyPartCommand* command = new CopyPartCommand(name);
            if (command->ok) MainWindow::Instance()->undoStack()->push(command);
            else delete command;
        }
        else if (type==ASSET_TYPE_COMPOSITE){
            CopyCompositeCommand* command = new CopyCompositeCommand(name);
            if (command->ok) MainWindow::Instance()->undoStack()->push(command);
            else delete command;
        }
    }
}

void PartList::deleteAsset()
{
    QListWidgetItem* item = mAssetListWidget->currentItem();
    if (item){
        int id = item->data(Qt::UserRole).toInt();
        const QString& name = mAssetListWidget->assetName(id);
        int type = mAssetListWidget->assetType(id);
        if (type==ASSET_TYPE_PART){
            DeletePartCommand* command = new DeletePartCommand(name);
            if (command->ok) MainWindow::Instance()->undoStack()->push(command);
            else delete command;
        }
        else if (type==ASSET_TYPE_COMPOSITE){
            DeleteCompositeCommand* command = new DeleteCompositeCommand(name);
            if (command->ok) MainWindow::Instance()->undoStack()->push(command);
            else delete command;
        }
    }
}

void PartList::sortAssets(){
    mAssetListWidget->sortItems();
}

void PartList::updateList(){
    mAssetListWidget->updateList();
}

void PartList::setSelection(const QString& name, int type){
    for(int i=0;i<mAssetListWidget->count();i++){
        QListWidgetItem* item = mAssetListWidget->item(i);
        int id = item->data(Qt::UserRole).toInt();
        if (type==mAssetListWidget->assetType(id) && name==mAssetListWidget->assetName(id)){
            // select it
            mAssetListWidget->setCurrentRow(i);
            return;
        }
    }
}
