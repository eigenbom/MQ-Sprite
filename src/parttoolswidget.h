#ifndef PARTTOOLSWIDGET_H
#define PARTTOOLSWIDGET_H

#include "resizemodedialog.h"
#include <QWidget>
#include <QToolButton>
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QStandardItemModel>
#include <QTreeView>
#include <QDebug>
#include <QMimeData>

class AnimatorWidget;
class PartWidget;
class PaletteView;

class LayerItemModel: public QStandardItemModel {
    Q_OBJECT
  public:

    LayerItemModel(QWidget* parent=nullptr):QStandardItemModel(parent){}
    Qt::DropActions supportedDropActions() const override {
        return Qt::MoveAction;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
     {
         Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

         if (index.isValid())
             return Qt::ItemIsDragEnabled | defaultFlags;
         else
             return Qt::ItemIsDropEnabled | defaultFlags;
     }

    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent){
        if (action==Qt::MoveAction){
            QByteArray encoded = data->data("application/x-qabstractitemmodeldatalist");
            QDataStream stream(&encoded, QIODevice::ReadOnly);
            int frow, fcol;
            QMap<int,  QVariant> roleDataMap;
            stream >> frow >> fcol >> roleDataMap;
            // qDebug() << frow << fcol << roleDataMap;

            // Dropping layer index into row
            // qDebug() << "Dropping something into row,col: " << row << "," << column;

            QModelIndex fromIndex = createIndex(frow, fcol);

            // Create the operation and then get updated later..


            /*
            // Move row
            QStandardItem* item = item(fromIndex);
            QModelIndex toIndex = createIndex(row, column);
            qDebug() << fromIndex << toIndex;
            */


            // qDebug() << data->
            // return QStandardItemModel::dropMimeData(data, action, row, column, parent);

            return true;
        }
        else return false;
    }

    signals:
    // void layersReorganised();
};

namespace Ui {
class PartToolsWidget;
}

class PartToolsWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit PartToolsWidget(QWidget *parent = 0);
    ~PartToolsWidget();
    PartWidget* targetPartWidget();
    void setTargetPartWidget(PartWidget* p); // if p is null then no target widget
    
public slots:
    void chooseColour();
    void chooseColourByText(QString);
    void colourSelected(QColor);
    void textPropertiesEdited();
    void penChanged();
    void zoomChanged();
    void targetPartNumFramesChanged();
    void targetPartNumPivotsChanged();
    void targetPartModesChanged();
    void targetPartLayersChanged();
    void targetPartPropertiesChanged();

    void setToolTypeDraw();
    void setToolTypeErase();
    void setToolTypePickColour();
    void setToolTypeStamp();
    void setToolTypeCopy();
    void setToolTypeFill();

    void setNumPivots(int);

    void modeActivated(QString);

    void addMode();
    void copyMode();
    void deleteMode();
    void renameMode();
    void resizeMode();
    void resizeModeDialogAccepted();

    void addPalette();
    void deletePalette();
    void paletteActivated(QString str);

    void selectNextMode();
    void selectPreviousMode();

    void layerClicked(const QModelIndex& index);
    void layerItemChanged(QStandardItem* item);

private:
    Ui::PartToolsWidget *ui;
    PartWidget* mTarget;
    AnimatorWidget* mAnimatorWidget;
    QTreeView* mLayerListView;
    LayerItemModel* mLayerItemModel;

    QColor mPenColour;
    QToolButton* mToolButtonColour;
    QLineEdit* mLineEditColour;
    QPlainTextEdit* mTextEditProperties;
    QComboBox* mComboBoxModes;
    QPushButton* mPushButtonModeSize;    

    QComboBox* mComboBoxPalettes;
    PaletteView* mPaletteView;
    QStringList mDefaultPalettes;

    ResizeModeDialog* mResizeModeDialog;

    QAction *mActionDraw, *mActionErase, *mActionPickColour, *mActionStamp, *mActionCopy, *mActionFill;
};

#endif // PARTTOOLSWIDGET_H
