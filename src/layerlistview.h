#ifndef LAYERLISTVIEW_H
#define LAYERLISTVIEW_H

//
// All this stuff is UNUSED
//

#include <QTreeView>
#include <QAbstractListModel>

class LayerListModel: public QAbstractListModel {
    Q_OBJECT
public:
    LayerListModel(QWidget* parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QModelIndex	parent(const QModelIndex & index) const;

    Qt::ItemFlags flags ( const QModelIndex & index ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const { Q_UNUSED(parent); return 2; }

protected:
    // Data
    struct Data {
        QString layerName;
        bool visible;
    };

    QList<Data> mData;
};

class LayerListView : public QTreeView
{
    Q_OBJECT
public:
    explicit LayerListView(QWidget *parent = 0);


signals:

public slots:

};

#endif // LAYERLISTVIEW_H
