#ifndef PARTLIST_H
#define PARTLIST_H

#include "assettreewidget.h"
#include <QWidget>

namespace Ui {
class PartList;
}

class PartList : public QWidget
{
    Q_OBJECT
    
public:
    explicit PartList(QWidget *parent = nullptr);
    ~PartList();

	void resetIcons();
	void updateList();
	void deselectAsset();
	void selectAsset(AssetRef ref);
	void updateIcon(AssetRef ref);

signals:
    void assetDoubleClicked(AssetRef ref);
	void assetSelected(AssetRef ref);

private:
    Ui::PartList *ui;
    AssetTreeWidget* mAssetTreeWidget;
};

#endif // PARTLIST_H
