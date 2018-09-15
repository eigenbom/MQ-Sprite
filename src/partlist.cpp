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
	connect(mAssetTreeWidget, SIGNAL(assetSelected(AssetRef)), this, SIGNAL(assetSelected(AssetRef)));
    updateList();
}

PartList::~PartList()
{
    delete ui;
}

void PartList::updateList(){
    mAssetTreeWidget->updateList();
}

void PartList::deselectAsset() {
	for (auto* item : mAssetTreeWidget->selectedItems()) {
		mAssetTreeWidget->setItemSelected(item, false);
	}
}

void PartList::selectAsset(AssetRef ref) {
	mAssetTreeWidget->selectAsset(ref);
}