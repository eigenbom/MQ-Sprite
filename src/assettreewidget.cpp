#include "assettreewidget.h"
#include "projectmodel.h"
#include "commands.h"
#include "mainwindow.h"

#include <QEvent>
#include <QtWidgets>

static QIcon createIcon(Part* part) {
	Q_ASSERT(part);

	QStringList modeList{ "icon", "side", "wrld" };
	modeList.append(part->modes.keys());
	for (const auto& mode : modeList) {
		if (part->modes.contains(mode)) {
			auto img = part->modes[mode].frames[0];
			// Extract a subregion from it
			// Auto-crop?								
			int cropLeft = img->width();
			int cropTop = img->height();
			int cropRight = 0;
			int cropBottom = 0;
			for (int x = 0; x < img->width(); ++x) {
				for (int y = 0; y < img->height(); ++y) {
					const bool opaque = (img->pixelColor(x, y).alpha() > 0);
					if (opaque) {
						if (cropLeft > x) cropLeft = x;
						if (cropRight < x) cropRight = x;
						if (cropTop > y) cropTop = y;
						if (cropBottom < y) cropBottom = y;
					}
				}
			}

			/*
			// TODO: Use the anchor to refine the crop selection
			QPoint anchor = part->modes[mode].anchor[0];

			int left = img->width() / 2 - 16;
			int top = img->height() / 2 - 16;
			int width = 32;
			int height = 32;
			if (mode == "side" || mode == "wrld") {
				left = anchor.x() - width / 2;
				top = anchor.y() - height;
			}*/

			int left = cropLeft;
			int top = cropTop;
			int width = 1 + cropRight - cropLeft;
			int height = 1 + cropBottom - cropTop;

			if (width < 8) {
				int expand = 8 - width;
				left -= expand / 2;
				width += expand;
			}

			if (height < 8) {
				int expand = 8 - height;
				top -= expand / 2;
				height += expand;
			}

			if (width > 2 && height > 2) {
				QImage copy = img->copy(left, top, width, height);
				int opaquePixelCount = 0;
				for (int x = 0; x < copy.width(); ++x) {
					for (int y = 0; y < copy.height(); ++y) {
						opaquePixelCount += (int)(copy.pixelColor(x, y).alpha() > 0);
					}
				}
				if (opaquePixelCount > 0.1 * copy.width() * copy.height()) {
					QIcon icon{ QPixmap::fromImage(copy.scaled(QSize(16, 16))) };
					return icon;
				}
			}
		}
	}

	return {};
}


AssetTreeWidget::AssetTreeWidget(QWidget *parent):QTreeWidget(parent)
{
    connect(this, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(activateItem(QTreeWidgetItem*,int)));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(expandItem(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(collapseItem(QTreeWidgetItem*)));
	connect(this, &QTreeWidget::itemSelectionChanged, [this]() {
		AssetRef ref;
		for (auto* item : selectedItems()) {
			int id = item->data(0, Qt::UserRole).toInt();
			ref = assetRef(id);
		}
		emit(assetSelected(ref));
	});

    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDropIndicatorShown(true);
    setAcceptDrops(true);
	setExpandsOnDoubleClick(false);
}

Qt::DropActions AssetTreeWidget::supportedDropActions() const
{
    return Qt::MoveAction;
}

AssetRef AssetTreeWidget::assetRef(int id) const {
    Q_ASSERT(id>=0 && id<mAssetRefs.size());
    return mAssetRefs.at(id);
}

const QString& AssetTreeWidget::assetName(int id) const {
    Q_ASSERT(id>=0 && id<mAssetNames.size());
    return mAssetNames.at(id);
}

bool AssetTreeWidget::selectAsset(AssetRef ref, QTreeWidgetItem* fromNode) {
	if (fromNode == nullptr) {
		bool selected = false;
		for (int i = 0; i < topLevelItemCount(); ++i) {
			selected = selectAsset(ref, topLevelItem(i)) || selected;
		}
		return selected;
	}
	else {
		const int index = fromNode->data(0, Qt::UserRole).toInt();
		const bool parentSelected = (ref == assetRef(index));
		setItemSelected(fromNode, parentSelected);		
		bool childSelected = false;
		for (int i = 0; i < fromNode->childCount(); ++i) {
			childSelected = selectAsset(ref, fromNode->child(i)) || childSelected;
		}
		if (childSelected) {
			fromNode->setExpanded(true);
		}
		return childSelected || parentSelected;
	}
}

void AssetTreeWidget::addAssetsWithParent(const QList<AssetRef>& assets, AssetRef parentRef, QTreeWidgetItem* parentItem, int& index){	       
    for(const AssetRef& ref: assets){
        Asset* asset = PM()->getAsset(ref);
        if (asset->parent == parentRef){
            if (ref.type==AssetType::Folder){
                QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);				
				
                item->setText(0, asset->name);
                item->setData(0, Qt::UserRole, index++);
                // item->setIcon(0, QIcon(":/icon/icons/folder.png"));
                item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
				item->setFlags(item->flags() | Qt::ItemIsEditable);
				item->setIcon(0, QIcon(":/icon/icons/gentleface/folder_icon&16.png"));
                mAssetRefs.push_back(asset->ref);
                mAssetNames.push_back(asset->name);
                addAssetsWithParent(assets, asset->ref, item, index);
                if (mOpenFolders.contains(asset->ref)){
                    item->setExpanded(true);
                }
            }
            else if (ref.type==AssetType::Part){
                QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
                item->setText(0, asset->name);
                item->setData(0, Qt::UserRole, index++);
				item->setFlags(item->flags() ^ Qt::ItemIsDropEnabled | Qt::ItemIsEditable);

				if (mAssetIcons.contains(asset->ref)) {
					item->setIcon(0, mAssetIcons[asset->ref]);
				}
				else {
					item->setIcon(0, QIcon{ ":/icon/icons/gentleface/picture_icon&16.png" });

					auto* part = PM()->getPart(asset->ref);
					if (part) {
						QIcon icon = createIcon(part);
						if (!icon.isNull()) {
							mAssetIcons.insert(asset->ref, icon);
							item->setIcon(0, icon);
						}
					}
				}

                mAssetRefs.push_back(asset->ref);
                mAssetNames.push_back(asset->name);
            }
            else if (ref.type==AssetType::Composite){
                QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
                item->setText(0, asset->name);
                item->setData(0, Qt::UserRole, index++);
                // item->setIcon(0, QIcon(":/icon/icons/composite.png"));
                item->setFlags(item->flags() ^ Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
                mAssetRefs.push_back(asset->ref);
                mAssetNames.push_back(asset->name);
            }
        }
    }

    /*
    for(Asset* asset: PM()->folders.values()){
        if (asset->parent == parentRef){
            QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
            item->setText(0, asset->name);
            item->setData(0, Qt::UserRole, index++);
            item->setIcon(0, QIcon(":/icon/icons/folder.png"));
            // item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            mAssetRefs.push_back(asset->ref);
            mAssetNames.push_back(asset->name);
            addAssetsWithParent(asset->ref, item, index);
            if (mOpenFolders.contains(asset->ref)){
                item->setExpanded(true);
            }
        }
    }

    for(Asset* asset: PM()->parts.values()){
        if (asset->parent == parentRef){
            QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
            item->setText(0, asset->name);
            item->setData(0, Qt::UserRole, index++);
            item->setIcon(0, QIcon(":/icon/icons/sprite.png"));
            item->setFlags(item->flags() ^ Qt::ItemIsDropEnabled);
            mAssetRefs.push_back(asset->ref);
            mAssetNames.push_back(asset->name);
        }
    }

    for(Asset* asset: PM()->composites.values()){
        if (asset->parent == parentRef){
            QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
            item->setText(0, asset->name);
            item->setData(0, Qt::UserRole, index++);
            item->setIcon(0, QIcon(":/icon/icons/composite.png"));
            item->setFlags(item->flags() ^ Qt::ItemIsDropEnabled);
            mAssetRefs.push_back(asset->ref);
            mAssetNames.push_back(asset->name);
        }
    }
    */
}

void AssetTreeWidget::updateList(){
    mAssetRefs.clear();
    mAssetNames.clear();
    this->clear();

    int index = 0;

    // Order the assets by type then by name
    QList<AssetRef> assets;

	auto sortByName = [](AssetRef a, AssetRef b) {return PM()->getAsset(a)->name < PM()->getAsset(b)->name; };

    auto l = PM()->folders.keys();
    qSort(l.begin(), l.end(), sortByName);
	assets += l;

	l = PM()->composites.keys();
	qSort(l.begin(), l.end(), sortByName);
	assets += l;

	l = PM()->parts.keys();
	qSort(l.begin(), l.end(), sortByName);
    assets += l;

    // Add everything recursively
	disconnect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(changeItem(QTreeWidgetItem*, int)));
    addAssetsWithParent(assets, AssetRef(), this->invisibleRootItem(), index);
	connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(changeItem(QTreeWidgetItem*, int)));
}


void AssetTreeWidget::updateIcon(AssetRef ref) {
	if (ref.type == AssetType::Part) {
		auto* item = findItem([&, ref](QTreeWidgetItem* item) -> bool {
			int index = item->data(0, Qt::UserRole).toInt();
			if (mAssetRefs.at(index) == ref) return true;
			else return false;
		});
		if (item) {
			Part* asset = PM()->getPart(ref);
			Q_ASSERT(asset->ref == ref);

			mAssetIcons.remove(ref);
			QIcon icon = createIcon(asset);
			if (!icon.isNull()) {
				mAssetIcons.insert(asset->ref, icon);
				item->setIcon(0, icon);
			}
		}
	}
}

void AssetTreeWidget::activateItem(QTreeWidgetItem* item, int){
    int index = item->data(0, Qt::UserRole).toInt();
    emit assetDoubleClicked(mAssetRefs.at(index));
}

void AssetTreeWidget::expandItem(QTreeWidgetItem* item){
    int index = item->data(0, Qt::UserRole).toInt();
    mOpenFolders.insert(mAssetRefs[index]);
}

void AssetTreeWidget::collapseItem(QTreeWidgetItem* item){
    int index = item->data(0, Qt::UserRole).toInt();
    mOpenFolders.remove(mAssetRefs[index]);
}

void AssetTreeWidget::changeItem(QTreeWidgetItem* item, int col) {
	if (col == 0) {
		int id = item->data(0, Qt::UserRole).toInt();
		AssetRef ref = assetRef(id);		
		auto oldName = assetName(id);
		auto newName = item->text(col);
		if (newName.isEmpty()) {
			// Reset to old name
			blockSignals(true);
			item->setText(col, oldName);
			blockSignals(false);
		}
		else if (oldName != newName) {
			switch (ref.type) {
			case AssetType::Folder: TryCommand(new CRenameFolder(ref, newName)); break;
			case AssetType::Part: TryCommand(new CRenamePart(ref, newName)); break;
			case AssetType::Composite: TryCommand(new CRenameComposite(ref, newName)); break;
			}
			MainWindow::Instance()->partListChanged();
		}
	}
}

bool AssetTreeWidget::filterItem(const QString& filterString, QTreeWidgetItem* item) {	
	if (item == nullptr) {
		bool visible = false;
		for (int i = 0; i < topLevelItemCount(); ++i) {
			visible = filterItem(filterString, topLevelItem(i)) || visible;
		}
		return visible;
	}
	else {
		const auto& itemText = item->text(0);
		bool visible = true;
		if (!filterString.isEmpty()) {
			visible = (itemText.indexOf(filterString, 0, Qt::CaseInsensitive) != -1);
		}
		bool childVisible = false;
		for (int i = 0; i < item->childCount(); ++i) {
			childVisible = filterItem(filterString, item->child(i)) || childVisible;
		}
		visible = visible || childVisible;
		item->setHidden(!visible);
		return visible;
	}
}

void AssetTreeWidget::setFilter(const QString& filterText) {
	auto text = filterText.trimmed();
	filterItem(text, nullptr);
}

void AssetTreeWidget::dropEvent(QDropEvent *event)
{
    AssetTreeWidget *source = qobject_cast<AssetTreeWidget *> (event->source());
    if (source) {
        const QMimeData* qMimeData = event->mimeData();
        QByteArray encoded = qMimeData->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        // Get target
        QTreeWidgetItem* dropIntoItem = this->itemAt(event->pos());
        AssetRef dropIntoRef;
        if (dropIntoItem){
            int dropInto = dropIntoItem->data(0, Qt::UserRole).toInt();
            dropIntoRef = mAssetRefs[dropInto];
        }

        while (!stream.atEnd())
        {
            int row, col;
            QMap<int,  QVariant> roleDataMap;            
            stream >> row >> col >> roleDataMap;
            // qDebug() << roleDataMap;

            // QString dropped = roleDataMap[0].toString();
            int index = roleDataMap[256].toInt();
            AssetRef ref = mAssetRefs[index];
            Asset* asset = PM()->getAsset(ref);

            // row
            // qDebug() << "dropped: " << ref.uuid << " into: " << dropIntoRef.uuid; // " into " << dropped;

            if (dropIntoRef.isNull()){
                if (!asset->parent.isNull()){
                    TryCommand(new CMoveAsset(ref, dropIntoRef));
                }
            }
            else if (dropIntoRef.type==AssetType::Folder){
                if (asset->parent!=dropIntoRef){
                    TryCommand(new CMoveAsset(ref, dropIntoRef));
                }
            }
            else {
                Asset* sibling = PM()->getAsset(dropIntoRef);
                Q_ASSERT(sibling);
                if (asset->parent != sibling->parent){
                    TryCommand(new CMoveAsset(ref, sibling->parent));
                }
            }
        }

        MainWindow::Instance()->partListChanged();
    }

    // QTreeWidget::dropEvent(event);
}

void AssetTreeWidget::keyPressEvent(QKeyEvent* event){
    if (event->key()==Qt::Key_Delete || event->key()==Qt::Key_Backspace){
        for(auto* item: selectedItems()){
            if (item){
                int id = item->data(0, Qt::UserRole).toInt();
                AssetRef ref = assetRef(id);
                switch(ref.type){
                case AssetType::Part: TryCommand(new CDeletePart(ref)); break;
                case AssetType::Composite: TryCommand(new CDeleteComposite(ref)); break;
                case AssetType::Folder: TryCommand(new CDeleteFolder(ref)); break;
                }
            }
        }
        MainWindow::Instance()->partListChanged();
    }
    else {
        QTreeWidget::keyPressEvent(event);
    }
}

QTreeWidgetItem* AssetTreeWidget::findItem(std::function<bool(QTreeWidgetItem*)> searchQuery, QTreeWidgetItem* parent) {
	if (parent == nullptr) {
		for (int i = 0; i < topLevelItemCount(); ++i) {
			auto* item = topLevelItem(i);
			if (searchQuery(item)) return item;
		}

		return nullptr;
	}
	else {
		if (searchQuery(parent)) return parent;
		for (int i = 0; i < parent->childCount(); ++i) {
			auto* item = parent->child(i);
			if (searchQuery(item)) return item;
		}

		return nullptr;
	}
}

/*

void AssetTreeWidget::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "AssetTreeWidget - MousePressEvent";
    if (event->button() == Qt::LeftButton) {
        mStartPos = event->pos();
    }

    QTreeWidget::mousePressEvent(event);
}

void AssetTreeWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        int distance = (event->pos() - mStartPos).manhattanLength();
        if (distance >= QApplication::startDragDistance())
            performDrag();
    }

    QTreeWidget::mouseMoveEvent(event);
}

void AssetTreeWidget::performDrag()
{
    qDebug() << "AssetTreeWidget - performDrag";
    QTreeWidgetItem *item = currentItem();

    if (item) {
        int index = item->data(0, Qt::UserRole).toInt();
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(QString::number(index));

        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);

        switch (mAssetRefs[index].type){
            case AssetType::Part: drag->setPixmap(QPixmap(":/icon/icons/sprite.png")); break;
            case AssetType::Composite: drag->setPixmap(QPixmap(":/icon/icons/composite.png")); break;
            case AssetType::Folder: drag->setPixmap(QPixmap(":/icon/icons/folder.png")); break;
        }

        / *
        QLabel label("hello");
        / *
        switch (mAssetRefs[index].type){
            case AssetType::Part: label.setPixmap(QPixmap(":/icon/icons/sprite.png")); break;
            case AssetType::Composite: label.setPixmap(QPixmap(":/icon/icons/composite.png")); break;
            case AssetType::Folder: label.setPixmap(QPixmap(":/icon/icons/folder.png")); break;
        }
        * /
        QPixmap pixmap(label.size());
        label.render(&pixmap);
        drag->setPixmap(pixmap);
        * /

        drag->exec(Qt::MoveAction);
    }
}

void AssetTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "AssetTreeWidget - DragEnterEvent";
    AssetTreeWidget *source = qobject_cast<AssetTreeWidget *>(event->source());
    if (source) {
        if (event->mimeData()->hasFormat("text/plain")) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else {
            event->ignore();
        }
    }
}

void AssetTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    qDebug() << "AssetTreeWidget - DragMoveEvent";
    AssetTreeWidget *source = qobject_cast<AssetTreeWidget *>(event->source());
    if (source) {
        if(event->mimeData()->hasFormat("text/plain")) {
            / *
            QTreeWidgetItem *item = itemAt(event->pos());
            if(item) {
                qDebug() << "itemat: " << item->text(0);
                source->setStyleSheet(QString::fromUtf8("QTreeView::item:hover {\n"
                                                        "background-color: rgb(123, 45, 67);\n"
                                                        "}"));
            }
            else {
                //item->setBackgroundColor(0, QColor(Qt::blue));
            }
            * /
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else
            event->ignore();
    }
}

void AssetTreeWidget::dropEvent(QDropEvent *event)
{
    qDebug() << "CloudView - DropEvent";

    AssetTreeWidget *source = qobject_cast<AssetTreeWidget *> (event->source());
    if (source) {
        event->ignore();

        / *
        if (event->mimeData()->hasFormat("text/plain")) {
            QByteArray appData = event->mimeData()->data("file/x-temp-files");
            QDataStream dataStream(&appData, QIODevice::ReadOnly);
            QTreeWidgetItem item;
            QString str;
            dataStream >> str >> item;
            qDebug() << "str: " << str << "item: " << item.text(0);

            //copy item here - to write function
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else
            event->ignore();
            * /
    }
}

void AssetTreeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{

}*/
