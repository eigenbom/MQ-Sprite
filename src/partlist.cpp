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
    // mAssetListWidget = findChild<AssetListWidget*>("assetList");
    // connect(mAssetListWidget, SIGNAL(assetDoubleClicked(AssetRef)), this, SIGNAL(assetDoubleClicked(AssetRef)));

    mAssetTreeWidget = findChild<AssetTreeWidget*>("assetTree");
    connect(mAssetTreeWidget, SIGNAL(assetDoubleClicked(AssetRef)), this, SIGNAL(assetDoubleClicked(AssetRef)));

    connect(findChild<QToolButton*>("toolButtonNewPart"), SIGNAL(clicked()), this, SLOT(newPart()));
    connect(findChild<QToolButton*>("toolButtonNewComp"), SIGNAL(clicked()), this, SLOT(newComp()));
    connect(findChild<QToolButton*>("toolButtonNewFolder"), SIGNAL(clicked()), this, SLOT(newFolder()));
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
    TryCommand(new CNewPart());
}

void PartList::newComp()
{
    TryCommand(new CNewComposite());
}

void PartList::newFolder()
{
    TryCommand(new CNewFolder());
}

void PartList::renameAsset()
{
    QTreeWidgetItem* item = mAssetTreeWidget->currentItem();
    if (item){
        int id = item->data(0, Qt::UserRole).toInt();
        AssetRef ref = mAssetTreeWidget->assetRef(id);
        const QString& name = mAssetTreeWidget->assetName(id);
        bool ok;
        QString text = QInputDialog::getText(this, tr("Rename Asset"),
                                             tr("Rename ") + name + ":", QLineEdit::Normal,
                                             name, &ok);
        if (ok && !text.isEmpty()){
            switch(ref.type){
            case AssetType::Part: TryCommand(new CRenamePart(ref, text)); break;
            case AssetType::Composite: TryCommand(new CRenameComposite(ref, text)); break;
            case AssetType::Folder: TryCommand(new CRenameFolder(ref, text)); break;
            }
        }
    }
}

void PartList::copyAsset()
{

    for(QTreeWidgetItem* item: mAssetTreeWidget->selectedItems()){
        if (item){
            int id = item->data(0, Qt::UserRole).toInt();
            AssetRef ref = mAssetTreeWidget->assetRef(id);
            switch(ref.type){
            case AssetType::Part: TryCommand(new CCopyPart(ref)); break;
            case AssetType::Composite: TryCommand(new CCopyComposite(ref)); break;
            case AssetType::Folder: qDebug() << "TODO: Copy folder"; break;
                // case AssetType::Folder: TryCommand(new CCopyFolder(ref)); break;
            }
        }
    }
}

void PartList::deleteAsset()
{
    for(QTreeWidgetItem* item: mAssetTreeWidget->selectedItems()){
        if (item){
            int id = item->data(0, Qt::UserRole).toInt();
            AssetRef ref = mAssetTreeWidget->assetRef(id);
            switch(ref.type){
            case AssetType::Part: TryCommand(new CDeletePart(ref)); break;
            case AssetType::Composite: TryCommand(new CDeleteComposite(ref)); break;
            case AssetType::Folder: TryCommand(new CDeleteFolder(ref)); break;
            }
        }
    }
}

void PartList::sortAssets(){
    // mAssetListWidget->sortItems();
}

void PartList::updateList(){
    // mAssetListWidget->updateList();
    mAssetTreeWidget->updateList();
}

void PartList::setSelection(AssetRef){
    // qDebug() << "TODO: setSelection: ref";

    /*
    for(int i=0;i<mAssetListWidget->count();i++){
        QListWidgetItem* item = mAssetListWidget->item(i);
        int id = item->data(Qt::UserRole).toInt();
        if (ref==mAssetListWidget->assetRef(id)){
            // select it
            mAssetListWidget->setCurrentRow(i);
            return;
        }
    }*/
}
