#ifndef ASSETTREEWIDGET_H
#define ASSETTREEWIDGET_H

#include <QTreeWidget>
#include <QVector>
#include <QSet>
#include <QString>
#include "projectmodel.h"

class AssetTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit AssetTreeWidget(QWidget *parent = 0);

    AssetRef assetRef(int id) const;
    const QString& assetName(int id) const;

public slots:
    void updateList();
    // void addAsset(AssetRef ref);
    // void removeAsset(AssetRef ref);

    void activateItem(QTreeWidgetItem* item, int col);
    void expandItem(QTreeWidgetItem* item);
    void collapseItem(QTreeWidgetItem* item);

signals:
    void assetDoubleClicked(AssetRef ref);

protected:
    Qt::DropActions supportedDropActions() const;
    void dropEvent(QDropEvent *event);
    void addAssetsWithParent(const QList<AssetRef>& assets, AssetRef parentRef, QTreeWidgetItem* parentItem, int& index);
    void keyPressEvent(QKeyEvent* event);

protected:
    QVector<AssetRef> mAssetRefs;
    // QVector<QTreeWidgetItem*> mItems;
    QVector<QString> mAssetNames;
    QSet<AssetRef> mOpenFolders;
    QPointF mStartPos;
};

#endif // ASSETTREEWIDGET_H
