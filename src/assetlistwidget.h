#ifndef ASSETLISTWIDGET_H
#define ASSETLISTWIDGET_H

#include <QListWidget>
#include <QVector>
#include <QString>
#include "projectmodel.h"

/// @deprecated use AssetTreeWidget instead
class AssetListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit AssetListWidget(QWidget *parent = 0);
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

#endif // ASSETLISTWIDGET_H
