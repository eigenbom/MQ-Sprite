#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QMap>
#include <QDebug>
#include "projectmodel.h"

class Command: public QUndoCommand {
public:
    bool ok; // is true if command can be processed    
};

bool TryCommand(Command* command); // execute a command if its ok. takes ownership.

class NewPartCommand: public Command
{
public:
    NewPartCommand();
    void undo();
    void redo();

private:
    AssetRef mUuid;
};

class CopyPartCommand: public Command
{
public:
    CopyPartCommand(AssetRef ref);
    void undo();
    void redo();

private:
    AssetRef mOriginal;
    AssetRef mCopy;
    QString mName;
    QString mNewPartName;
};

class DeletePartCommand: public Command {
public:    
    DeletePartCommand(AssetRef ref);
    ~DeletePartCommand();
    void undo();
    void redo();

private:
    AssetRef mRef;
    Part* mCopy;
};

class RenamePartCommand: public Command {
public:
    RenamePartCommand(AssetRef ref, QString newName);
    void undo();
    void redo();

private:
    AssetRef mRef;
    QString mOldName, mNewName;
};


// Composite

class NewCompositeCommand: public Command
{
public:
    NewCompositeCommand();
    void undo();
    void redo();

private:
    AssetRef mRef;
    QString mName;
};

class CopyCompositeCommand: public Command
{
public:
    CopyCompositeCommand(AssetRef ref);
    void undo();
    void redo();

private:
    AssetRef mOriginal;
    AssetRef mCopy;
    QString mNewCompositeName;
};

class DeleteCompositeCommand: public Command {
public:
    DeleteCompositeCommand(AssetRef ref);
    ~DeleteCompositeCommand();
    void undo();
    void redo();

private:
    AssetRef mRef;
    Composite* mCopy;
};

class RenameCompositeCommand: public Command {
public:
    RenameCompositeCommand(AssetRef ref, QString newName);
    void undo();
    void redo();

private:
    AssetRef mRef;
    QString mOldName, mNewName;
};











class NewModeCommand: public Command
{
public:
    NewModeCommand(AssetRef part, const QString& copyModeName);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mCopyModeName;
    QString mModeName;
};

class DeleteModeCommand: public Command
{
public:
    DeleteModeCommand(AssetRef part, const QString& modeName);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    Part::Mode mModeCopy;
};

class ResetModeCommand: public Command
{
public:
    ResetModeCommand(AssetRef part, const QString& modeName);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    Part::Mode mModeCopy;
};

class CopyModeCommand: public Command
{
public:
    CopyModeCommand(AssetRef part, const QString& modeName);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    QString mNewModeName;
};

class RenameModeCommand: public Command
{
public:
    RenameModeCommand(AssetRef part, const QString& oldModeName, const QString& newModeName);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mOldModeName;
    QString mNewModeName;
};






class DrawOnPartCommand: public Command {
public:
    DrawOnPartCommand(AssetRef part, QString mode, int frame, QImage data, QPoint offset);
    void undo();
    void redo();
private:
    AssetRef mPart;
    QString mMode;
    int mFrame;
    QImage mData;
    QPoint mOffset;
    QImage mOldFrame;
};

class EraseOnPartCommand: public Command {
public:
    EraseOnPartCommand(AssetRef part, QString mode, int frame, QImage data, QPoint offset);
    void undo();
    void redo();
private:
    AssetRef mPart;
    QString mMode;
    int mFrame;
    QImage mData;
    QPoint mOffset;
    QImage mOldFrame;
};


class NewFrameCommand: public Command {
public:
    NewFrameCommand(AssetRef part, QString modeName, int index);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mIndex;
};

class CopyFrameCommand: public Command {
public:
    CopyFrameCommand(AssetRef part, QString modeName, int index);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mIndex;
};

class DeleteFrameCommand: public Command {
public:
    DeleteFrameCommand(AssetRef part, QString modeName, int index);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mIndex;
    QImage mImage;
    QPoint mAnchor;
    QPoint mPivots[MAX_PIVOTS];
};





class UpdateAnchorAndPivotsCommand: public Command {
public:
    UpdateAnchorAndPivotsCommand(AssetRef part, QString modeName, int index, QPoint anchor, QPoint p1, QPoint p2, QPoint p3, QPoint p4);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mIndex;
    QPoint mAnchor, mPivots[MAX_PIVOTS];
    QPoint mOldAnchor, mOldPivots[MAX_PIVOTS];
};

class ChangeNumPivotsCommand: public Command {
public:
    ChangeNumPivotsCommand(AssetRef part, QString modeName, int numPivots);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mIndex;
    int mNumPivots;
    int mOldNumPivots;
};

class ChangeModeSizeCommand: public Command {
public:
    ChangeModeSizeCommand(AssetRef part, QString modeName, int width, int height, int offsetX, int offsetY);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mOldWidth, mOldHeight;
    int mWidth, mHeight, mOffsetX, mOffsetY;
    Part::Mode mOldMode;
};

class ChangeModeFPSCommand: public Command {
public:
    ChangeModeFPSCommand(AssetRef part, QString modeName, int fps);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mOldFPS;
    int mFPS;
};




class EditCompositeChildCommand: public Command {
public:
    EditCompositeChildCommand(AssetRef comp, const QString& childName, const QString& newPartName, int newZ, int newParent, int newParentPivot);
    void undo();
    void redo();

private:
    AssetRef mComp;
    QString mChildName;
    QString mOldPartName, mNewPartName;    
    int mOldParent, mNewParent;
    int mOldParentPivot, mNewParentPivot;
    int mOldRoot;
    int mOldZ, mNewZ;
};

class NewCompositeChildCommand: public Command {
public:
    NewCompositeChildCommand(AssetRef comp);
    void redo();
    void undo();

private:
    AssetRef mComp;
    QString mChildName;
};

class EditCompositeChildNameCommand: public Command {
public:
    EditCompositeChildNameCommand(AssetRef comp, const QString& child, const QString& newChildName);
    void redo();
    void undo();

private:
    AssetRef mComp;
    QString mOldChildName, mNewChildName;
};

class DeleteCompositeChildCommand: public Command {
public:
    DeleteCompositeChildCommand(AssetRef comp, const QString& childName);
    void redo();
    void undo();

private:
    AssetRef mComp;
    QString mChildName;
    int mChildIndex;
    Composite mCompCopy;
};

class ChangePropertiesCommand: public Command {
public:
    ChangePropertiesCommand(AssetRef part, QString properties);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mProperties;
    QString mOldProperties;
};

class ChangeCompPropertiesCommand: public Command {
public:
    ChangeCompPropertiesCommand(AssetRef comp, QString properties);
    void undo();
    void redo();

private:
    AssetRef mComp;
    QString mProperties;
    QString mOldProperties;
};

#endif // COMMANDS_H
