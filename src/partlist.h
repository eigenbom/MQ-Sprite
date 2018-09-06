#ifndef PARTLIST_H
#define PARTLIST_H

#include <QWidget>
#include <QListWidget>

#include "assettreewidget.h"

namespace Ui {
class PartList;
}

class PartList : public QWidget
{
    Q_OBJECT
    
public:
    explicit PartList(QWidget *parent = nullptr);
    ~PartList();

signals:
    void assetDoubleClicked(AssetRef ref);

public slots:    
    void newPart();
    void newComp();
    void newFolder();
    void renameAsset();
    void copyAsset();
    void deleteAsset();
    void sortAssets();
    
    void updateList();

    void setSelection(AssetRef ref);

private:
    Ui::PartList *ui;
    // AssetListWidget* mAssetListWidget;
    AssetTreeWidget* mAssetTreeWidget;
};

#endif // PARTLIST_H
