#include "partlist.h"
#include "ui_partlist.h"

#include "mainwindow.h"
#include "commands.h"

#include <QtWidgets>

PartList::PartList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PartList){
    ui->setupUi(this);
    mAssetTreeWidget = findChild<AssetTreeWidget*>("assetTree");
    connect(mAssetTreeWidget, SIGNAL(assetDoubleClicked(AssetRef)), this, SIGNAL(assetDoubleClicked(AssetRef)));
    // connect(findChild<QToolButton*>("toolButtonNewPart"), SIGNAL(clicked()), this, SLOT(newPart()));
    // connect(findChild<QToolButton*>("toolButtonNewFolder"), SIGNAL(clicked()), this, SLOT(newFolder()));
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

    MainWindow::Instance()->partListChanged();
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
