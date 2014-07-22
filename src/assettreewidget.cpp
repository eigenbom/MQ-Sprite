#include "assettreewidget.h"
#include "projectmodel.h"

#include <QEvent>
#include <QtWidgets>

AssetTreeWidget::AssetTreeWidget(QWidget *parent):QTreeWidget(parent)
{
    // viewport()->installEventFilter(this);
}

bool AssetTreeWidget::eventFilter(QObject *object, QEvent *event)
{
    /*
    if (object == viewport() && event->type()==QEvent::KeyPress){
        QKeyEvent* key = dynamic_cast<QKeyEvent*>(event);
        // qDebug() << key->key();
        return false;
    }
    else if (object == viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QListWidgetItem* item = this->currentItem();
        if (item){
            int index = item->data(Qt::UserRole).toInt();
            emit assetDoubleClicked(mAssetRefs.at(index), mAssetTypes.at(index));
        }
        return true;
    }
    */
    return false;
}

AssetRef AssetTreeWidget::assetRef(int id) const {
    Q_ASSERT(id>=0 && id<mAssetRefs.size());
    return mAssetRefs.at(id);
}

const QString& AssetTreeWidget::assetName(int id) const {
    Q_ASSERT(id>=0 && id<mAssetNames.size());
    return mAssetNames.at(id);
}

void AssetTreeWidget::updateList(){
    mAssetRefs.clear();
    mAssetNames.clear();
    this->clear();

    // QFont font;
    // QFont cfont;
    // cfont.setBold(true);

    int index = 0;

    foreach(AssetRef ref, ProjectModel::Instance()->parts.keys()){
        Part* part = PM()->getPart(ref);

        QTreeWidgetItem* item = new QTreeWidgetItem(this);
        item->setText(0, part->name);
        item->setData(0, Qt::UserRole, index++);
        //item->setFont(0, font);
        item->setIcon(0, QIcon(":/icon/icons/sprite.png"));
        // item->setBackgroundColor(0, QColor(255,255,240));

        mAssetRefs.push_back(part->ref);
        mAssetNames.push_back(part->name);

        this->addTopLevelItem(item);
    }
    foreach(AssetRef ref, ProjectModel::Instance()->composites.keys()){
        Composite* comp = PM()->getComposite(ref);

        QTreeWidgetItem* item = new QTreeWidgetItem(this);
        item->setText(0, comp->name);
        item->setData(0, Qt::UserRole, index++);
        //item->setFont(0, font); // cfont
        item->setIcon(0, QIcon(":/icon/icons/composite.png"));
        // item->setBackgroundColor(0, QColor(255,240,255));

        mAssetRefs.push_back(comp->ref);
        mAssetNames.push_back(comp->name);

        this->addTopLevelItem(item);
    }
    foreach(AssetRef ref, ProjectModel::Instance()->folders.keys()){
        Folder* folder = PM()->getFolder(ref);

        QTreeWidgetItem* item = new QTreeWidgetItem(this);
        item->setText(0, folder->name);
        item->setData(0, Qt::UserRole, index++);
        //item->setFont(0, font); // cfont
        item->setIcon(0, QIcon(":/icon/icons/folder.png"));
        // item->setBackgroundColor(0, QColor(230,230,230));
        item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

        mAssetRefs.push_back(folder->ref);
        mAssetNames.push_back(folder->name);

        this->addTopLevelItem(item);
    }
}
