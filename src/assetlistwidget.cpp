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
        // QKeyEvent* key = dynamic_cast<QKeyEvent*>(event);
        // qDebug() << key->key();
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
            emit assetDoubleClicked(mAssetRefs.at(index));
        }
        return true;
    }
    return false;
}

AssetRef AssetListWidget::assetRef(int id) const {
    Q_ASSERT(id>=0 && id<mAssetRefs.size());
    return mAssetRefs.at(id);
}

const QString& AssetListWidget::assetName(int id) const {
    Q_ASSERT(id>=0 && id<mAssetNames.size());
    return mAssetNames.at(id);
}

void AssetListWidget::updateList(){
    mAssetRefs.clear();
    mAssetNames.clear();
    this->clear();

    QFont font;
    QFont cfont;
    cfont.setBold(true);

    int index = 0;

    foreach(AssetRef ref, ProjectModel::Instance()->parts.keys()){
        Part* part = PM()->getPart(ref);

        QListWidgetItem* item = new QListWidgetItem(part->name);
        item->setData(Qt::UserRole, index++);
        item->setFont(font);
        item->setBackgroundColor(QColor(255,255,240));

        mAssetRefs.push_back(part->ref);
        mAssetNames.push_back(part->name);

        this->addItem(item);
    }
    foreach(AssetRef ref, ProjectModel::Instance()->composites.keys()){
        Composite* comp = PM()->getComposite(ref);

        QListWidgetItem* item = new QListWidgetItem(comp->name);
        item->setData(Qt::UserRole, index++);
        item->setFont(font); // cfont
        item->setBackgroundColor(QColor(255,240,255));

        mAssetRefs.push_back(comp->ref);
        mAssetNames.push_back(comp->name);

        this->addItem(item);
    }
    foreach(AssetRef ref, ProjectModel::Instance()->folders.keys()){
        Folder* folder = PM()->getFolder(ref);

        QListWidgetItem* item = new QListWidgetItem(folder->name);
        item->setData(Qt::UserRole, index++);
        item->setFont(font); // cfont
        item->setBackgroundColor(QColor(230,230,230));

        mAssetRefs.push_back(folder->ref);
        mAssetNames.push_back(folder->name);

        this->addItem(item);
    }
}
