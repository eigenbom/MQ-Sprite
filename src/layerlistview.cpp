#include "layerlistview.h"

LayerListModel::LayerListModel(QWidget* parent)
    :QAbstractListModel(parent)
{
    mData.append(Data{tr("Hidden Layer"), false});
    mData.append(Data{tr("Some Layer"), true});
    mData.append(Data{tr("Some Layer 2"), true});
    mData.append(Data{tr("Some Layer 3"), true});
}

QModelIndex LayerListModel::index(int row, int column, const QModelIndex & parent) const {
    Q_UNUSED(parent);
    return createIndex(row, column, nullptr);
}

QModelIndex	LayerListModel::parent(const QModelIndex & index) const {
    Q_UNUSED(index);
    return QModelIndex();
}

Qt::ItemFlags LayerListModel::flags ( const QModelIndex & index ) const {

    if (index.isValid()){
        if (index.column()==1){
            return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable; // | Qt::CheckStateRole;
        }
        else return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
    }
    else return QAbstractListModel::flags(index);
}

QVariant LayerListModel::data ( const QModelIndex & index, int role) const {
    if (index.isValid() && role == Qt::DisplayRole){
        const Data& data = mData[index.row()];
        if (index.column()==0) return data.layerName;
        else return data.visible;
    }
    else if (index.isValid() && role == Qt::CheckStateRole){
        const Data& data = mData[index.row()];

        if (index.column()==1) return data.visible;
    }

    return QVariant();
}

bool LayerListModel::setData(const QModelIndex & index, const QVariant & value, int role){
    if (index.isValid() && index.column()==0 && role == Qt::EditRole){
        mData[index.row()].layerName = value.toString();
        return true;
    }
    else if (index.isValid() && index.column()==1 && role == Qt::CheckStateRole){
        mData[index.row()].visible = value.toBool();
        return true;
    }
    else return false;
}

QVariant LayerListModel::headerData ( int section, Qt::Orientation orientation, int role) const {
    return QVariant();
}

int LayerListModel::rowCount ( const QModelIndex & parent) const {
    Q_UNUSED(parent);
    return mData.length();
}

LayerListView::LayerListView(QWidget *parent) :
    QTreeView(parent)
{
    setHeaderHidden(true);


}
