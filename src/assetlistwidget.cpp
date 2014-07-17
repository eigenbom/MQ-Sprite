#include "assetlistwidget.h"
#include "projectmodel.h"

#include <QEvent>
#include <QtWidgets>

AssetListWidget::AssetListWidget(QWidget *parent) :
    QListWidget(parent)
{
    viewport()->installEventFilter(this);
}

bool AssetListWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object == viewport() && event->type()==QEvent::KeyPress){
        QKeyEvent* key = dynamic_cast<QKeyEvent*>(event);
        qDebug() << key->key();
        /*
        if (mAssetView->selectionModel()->hasSelection() && key->key()==Qt::Key_Delete){
            // Delete
            deletePart();
            return true;
        }
        */
        return false;
    }
    else if (object == viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QListWidgetItem* item = this->currentItem();
        if (item){
            int index = item->data(Qt::UserRole).toInt();
            QString name = mAssetNames.at(index);
            int type = mAssetTypes.at(index);
            emit assetDoubleClicked(name,type);
        }
        return true;
    }
    return false;
}

const QString& AssetListWidget::assetName(int id) const {
    Q_ASSERT(id>=0 && id<mAssetNames.size());
    return mAssetNames.at(id);
}

int AssetListWidget::assetType(int id) const {
    Q_ASSERT(id>=0 && id<mAssetTypes.size());
    return mAssetTypes.at(id);
}

void AssetListWidget::updateList(){
    mAssetNames.clear();
    mAssetTypes.clear();
    this->clear();

    QFont font;
    QFont cfont;
    cfont.setBold(true);

    int index = 0;
    foreach(QString key, ProjectModel::Instance()->parts.keys()){
        QListWidgetItem* item = new QListWidgetItem(key);
        item->setData(Qt::UserRole, index++);
        item->setFont(font);
        item->setBackgroundColor(QColor(255,255,240));

        mAssetNames.push_back(key);
        mAssetTypes.push_back(ASSET_TYPE_PART);

        this->addItem(item);
    }
    foreach(QString key, ProjectModel::Instance()->composites.keys()){
        QListWidgetItem* item = new QListWidgetItem(key);
        item->setData(Qt::UserRole, index++);
        item->setFont(font); // cfont
        item->setBackgroundColor(QColor(255,240,255));

        mAssetNames.push_back(key);
        mAssetTypes.push_back(ASSET_TYPE_COMPOSITE);

        this->addItem(item);
    }
}
