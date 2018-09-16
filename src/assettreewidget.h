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
    explicit AssetTreeWidget(QWidget *parent = nullptr);

    AssetRef assetRef(int id) const;
    const QString& assetName(int id) const;
	bool selectAsset(AssetRef, QTreeWidgetItem* fromNode = nullptr);

public slots:
    void updateList();
	void updateIcon(AssetRef ref);
    // void addAsset(AssetRef ref);
    // void removeAsset(AssetRef ref);

    void activateItem(QTreeWidgetItem* item, int col);
    void expandItem(QTreeWidgetItem* item);
    void collapseItem(QTreeWidgetItem* item);
	void changeItem(QTreeWidgetItem* item, int col);

	void setFilter(const QString& filter);

	void toggleFolders();

signals:
    void assetDoubleClicked(AssetRef ref);
	void assetSelected(AssetRef ref);

protected:
    Qt::DropActions supportedDropActions() const;
    void dropEvent(QDropEvent *event);
    void addAssetsWithParent(const QList<AssetRef>& assets, AssetRef parentRef, QTreeWidgetItem* parentItem, int& index);
    void keyPressEvent(QKeyEvent* event);
	bool filterItem(const QString& text, QTreeWidgetItem* item);
	
	QTreeWidgetItem* findItem(std::function<bool(QTreeWidgetItem*)> searchQuery, QTreeWidgetItem* root = nullptr);
	void applyToAllItems(std::function<void(QTreeWidgetItem*)> function, QTreeWidgetItem* root = nullptr);

protected:
    QVector<AssetRef> mAssetRefs;
    // QVector<QTreeWidgetItem*> mItems;
    QVector<QString> mAssetNames;
    QSet<AssetRef> mOpenFolders;
    QPointF mStartPos;
	QMap<AssetRef, QIcon> mAssetIcons;
};

#endif // ASSETTREEWIDGET_H
