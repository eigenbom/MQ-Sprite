#ifndef ASSETTREEWIDGET_H
#define ASSETTREEWIDGET_H

#include <QTreeWidget>
#include <QVector>
#include <QString>
#include "projectmodel.h"

class AssetTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit AssetTreeWidget(QWidget *parent = 0);
    bool eventFilter(QObject *object, QEvent *event);
    AssetRef assetRef(int id) const;
    const QString& assetName(int id) const;

signals:
    void assetDoubleClicked(AssetRef ref);

public slots:
    void updateList();

protected:
    QVector<AssetRef> mAssetRefs;
    QVector<QString> mAssetNames;
};

#endif // ASSETTREEWIDGET_H
