#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QMap>
#include <QDebug>
#include "projectmodel.h"

// All undoable actions follow the Command pattern and inherit from QUndoCommand
// TODO: Can compress the undo stack by implementing mergesWith() for some commands
// Example command execution: TryCommand(new CRenamePart(ref, "New part name"));

class Command: public QUndoCommand {
public:
    bool ok; // is true if command can be processed    
};

bool TryCommand(Command* command); // execute a command if its ok. takes ownership.

class CNewPart: public Command
{
public:
    CNewPart();
    void undo();
    void redo();

private:
    AssetRef mUuid;
};

class CCopyPart: public Command
{
public:
    CCopyPart(AssetRef ref);
    void undo();
    void redo();

private:
    AssetRef mOriginal;
    AssetRef mCopy;
    QString mName;
    QString mNewPartName;
};

class CDeletePart: public Command {
public:    
    CDeletePart(AssetRef ref);
    void undo();
    void redo();

private:
    AssetRef mRef;
    QSharedPointer<Part> mCopy;
};

class CRenamePart: public Command {
public:
    CRenamePart(AssetRef ref, QString newName);
    void undo();
    void redo();

private:
    AssetRef mRef;
    QString mOldName, mNewName;
};


// Composite

class CNewComposite: public Command
{
public:
    CNewComposite();
    void undo();
    void redo();

private:
    AssetRef mRef;
    QString mName;
};

class CCopyComposite: public Command
{
public:
    CCopyComposite(AssetRef ref);
    void undo();
    void redo();

private:
    AssetRef mOriginal;
    AssetRef mCopy;
    QString mNewCompositeName;
};

class CDeleteComposite: public Command {
public:
    CDeleteComposite(AssetRef ref);
    void undo();
    void redo();

private:
    AssetRef mRef;
    QSharedPointer<Composite> mCopy;
};

class CRenameComposite: public Command {
public:
    CRenameComposite(AssetRef ref, QString newName);
    void undo();
    void redo();

private:
    AssetRef mRef;
    QString mOldName, mNewName;
};

// Folders

class CNewFolder: public Command
{
public:
    CNewFolder();
    void undo();
    void redo();

private:
    AssetRef mRef;
};

class CDeleteFolder: public Command {
public:
    CDeleteFolder(AssetRef ref);
    void undo();
    void redo();

private:
    AssetRef mRef;
    QSharedPointer<Folder> mCopy;
};

class CRenameFolder: public Command {
public:
    CRenameFolder(AssetRef ref, QString newName);
    void undo();
    void redo();

private:
    AssetRef mRef;
    QString mOldName, mNewName;
};


class CMoveAsset: public Command {
public:
    CMoveAsset(AssetRef ref, AssetRef newParent);
    void undo();
    void redo();
private:
    AssetRef mRef, mOldParent, mNewParent;
};






class CNewMode: public Command
{
public:
    CNewMode(AssetRef part, const QString& copyModeName);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mCopyModeName;
    QString mModeName;
};

class CDeleteMode: public Command
{
public:
    CDeleteMode(AssetRef part, const QString& modeName);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    Part::Mode mModeCopy;
};

class CResetMode: public Command
{
public:
    CResetMode(AssetRef part, const QString& modeName);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    Part::Mode mModeCopy;
};

class CCopyMode: public Command
{
public:
    CCopyMode(AssetRef part, const QString& modeName);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    QString mNewModeName;
};

class CRenameMode: public Command
{
public:
    CRenameMode(AssetRef part, const QString& oldModeName, const QString& newModeName);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mOldModeName;
    QString mNewModeName;
};






class CDrawOnPart: public Command {
public:
    CDrawOnPart(AssetRef part, QString mode, int frame, QImage data, QPoint offset);
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

class CEraseOnPart: public Command {
public:
    CEraseOnPart(AssetRef part, QString mode, int frame, QImage data, QPoint offset);
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


class CNewFrame: public Command {
public:
    CNewFrame(AssetRef part, QString modeName, int index);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mIndex;
};

class CCopyFrame: public Command {
public:
    CCopyFrame(AssetRef part, QString modeName, int index);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mIndex;
};

class CDeleteFrame: public Command {
public:
    CDeleteFrame(AssetRef part, QString modeName, int index);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mIndex;
    QSharedPointer<QImage> mImage;
    QPoint mAnchor;
    QPoint mPivots[MAX_PIVOTS];
};





class CUpdateAnchorAndPivots: public Command {
public:
    CUpdateAnchorAndPivots(AssetRef part, QString modeName, int index, QPoint anchor, QPoint p1, QPoint p2, QPoint p3, QPoint p4);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mIndex;
    QPoint mAnchor, mPivots[MAX_PIVOTS];
    QPoint mOldAnchor, mOldPivots[MAX_PIVOTS];
};

class CChangeNumPivots: public Command {
public:
    CChangeNumPivots(AssetRef part, QString modeName, int numPivots);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mIndex;
    int mNumPivots;
    int mOldNumPivots;
};

class CChangeModeSize: public Command {
public:
    CChangeModeSize(AssetRef part, QString modeName, int width, int height, int offsetX, int offsetY);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mOldWidth, mOldHeight;
    int mWidth, mHeight, mOffsetX, mOffsetY;
    Part::Mode mOldMode;
};

class CChangeModeFPS: public Command {
public:
    CChangeModeFPS(AssetRef part, QString modeName, int fps);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mModeName;
    int mOldFPS;
    int mFPS;
};




class CEditCompositeChild: public Command {
public:
    CEditCompositeChild(AssetRef comp, const QString& childName, AssetRef newPart, int newZ, int newParent, int newParentPivot);
    void undo();
    void redo();

private:
    AssetRef mComp;
    QString mChildName;
    AssetRef mOldPart, mNewPart;
    int mOldParent, mNewParent;
    int mOldParentPivot, mNewParentPivot;
    int mOldRoot;
    int mOldZ, mNewZ;
};

class CNewCompositeChild: public Command {
public:
    CNewCompositeChild(AssetRef comp);
    void redo();
    void undo();

private:
    AssetRef mComp;
    QString mChildName;
};

class CEditCompositeChildName: public Command {
public:
    CEditCompositeChildName(AssetRef comp, const QString& child, const QString& newChildName);
    void redo();
    void undo();

private:
    AssetRef mComp;
    QString mOldChildName, mNewChildName;
};

class CDeleteCompositeChild: public Command {
public:
    CDeleteCompositeChild(AssetRef comp, const QString& childName);
    void redo();
    void undo();

private:
    AssetRef mComp;
    QString mChildName;
    int mChildIndex;
    QSharedPointer<Composite> mCompCopy;
};

class CChangePartProperties: public Command {
public:
    CChangePartProperties(AssetRef part, QString properties);
    void undo();
    void redo();

private:
    AssetRef mPart;
    QString mProperties;
    QString mOldProperties;
};

class CChangeCompProperties: public Command {
public:
    CChangeCompProperties(AssetRef comp, QString properties);
    void undo();
    void redo();

private:
    AssetRef mComp;
    QString mProperties;
    QString mOldProperties;
};

#endif // COMMANDS_H
