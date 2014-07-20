#ifndef PARTLIST_H
#define PARTLIST_H

#include "assetlistwidget.h"
#include <QWidget>
#include <QListWidget>

namespace Ui {
class PartList;
}

class PartList : public QWidget
{
    Q_OBJECT
    
public:
    explicit PartList(QWidget *parent = 0);
    ~PartList();

signals:
    void assetDoubleClicked(AssetRef ref, AssetType type);

public slots:
    void newPart();
    void newComp();
    void renameAsset();
    void copyAsset();
    void deleteAsset();
    void sortAssets();
    
    void updateList();

    void setSelection(AssetRef ref, AssetType type);

private:
    Ui::PartList *ui;
    AssetListWidget* mAssetListWidget;
};

#endif // PARTLIST_H
