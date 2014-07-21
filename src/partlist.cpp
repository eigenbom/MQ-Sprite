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
    TryCommand(new CNewPart());
}

void PartList::newComp()
{
    TryCommand(new CNewComposite());
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
                TryCommand(new CRenamePart(ref, text));
            }
            else if (type==AssetType::Composite){
                TryCommand(new CRenameComposite(ref, text));
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
            TryCommand(new CCopyPart(ref));
        }
        else if (type==AssetType::Composite){
            TryCommand(new CCopyComposite(ref));
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
            TryCommand(new CDeletePart(ref));
        }
        else if (type==AssetType::Composite){
            TryCommand(new CDeleteComposite(ref));
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
