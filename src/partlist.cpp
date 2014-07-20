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
    connect(mAssetListWidget, SIGNAL(assetDoubleClicked(AssetRef,AssetType)), this, SIGNAL(assetDoubleClicked(AssetRef,AssetType)));
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
        AssetRef ref = mAssetListWidget->assetRef(id);
        const QString& name = mAssetListWidget->assetName(id);
        AssetType type = mAssetListWidget->assetType(id);
        bool ok;
        QString text = QInputDialog::getText(this, tr("Rename Asset"),
                                             tr("Rename ") + name + ":", QLineEdit::Normal,
                                             name, &ok);
        if (ok && !text.isEmpty()){
            if (type==AssetType::Part){
                RenamePartCommand* command = new RenamePartCommand(ref, text);
                if (command->ok){
                    MainWindow::Instance()->undoStack()->push(command);
                }
                else {
                    delete command;
                }
            }
            else if (type==AssetType::Composite){
                RenameCompositeCommand* command = new RenameCompositeCommand(ref, text);
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
        AssetRef ref = mAssetListWidget->assetRef(id);
        AssetType type = mAssetListWidget->assetType(id);
        if (type==AssetType::Part){
            CopyPartCommand* command = new CopyPartCommand(ref);
            if (command->ok) MainWindow::Instance()->undoStack()->push(command);
            else delete command;
        }
        else if (type==AssetType::Composite){
            CopyCompositeCommand* command = new CopyCompositeCommand(ref);
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

        AssetRef ref = mAssetListWidget->assetRef(id);
        AssetType type = mAssetListWidget->assetType(id);
        if (type==AssetType::Part){
            DeletePartCommand* command = new DeletePartCommand(ref);
            if (command->ok) MainWindow::Instance()->undoStack()->push(command);
            else delete command;
        }
        else if (type==AssetType::Composite){
            DeleteCompositeCommand* command = new DeleteCompositeCommand(ref);
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

void PartList::setSelection(AssetRef ref, AssetType type){
    for(int i=0;i<mAssetListWidget->count();i++){
        QListWidgetItem* item = mAssetListWidget->item(i);
        int id = item->data(Qt::UserRole).toInt();
        if (type==mAssetListWidget->assetType(id) && ref==mAssetListWidget->assetRef(id)){
            // select it
            mAssetListWidget->setCurrentRow(i);
            return;
        }
    }
}
