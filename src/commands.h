#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QMap>
#include <QDebug>
#include "projectmodel.h"

class Command: public QUndoCommand {
public:
    bool ok; // is true if command can be processed
    ProjectModel* PM(){return ProjectModel::Instance();}
};

class NewPartCommand: public Command
{
public:
    NewPartCommand();
    void undo();
    void redo();

private:
    QString mName;
};

class CopyPartCommand: public Command
{
public:
    CopyPartCommand(const QString& name);
    void undo();
    void redo();

private:
    QString mName;
    QString mNewPartName;
};

class DeletePartCommand: public Command {
public:
    DeletePartCommand(QString name);
    ~DeletePartCommand();
    void undo();
    void redo();

private:
    QString mName;
    Part* mCopy;
};

class RenamePartCommand: public Command {
public:
    RenamePartCommand(QString oldName, QString newName);
    void undo();
    void redo();

private:
    QString mOldName, mNewName;
};

class NewModeCommand: public Command
{
public:
    NewModeCommand(const QString& partName, const QString& copyModeName);
    void undo();
    void redo();

private:
    QString mPartName;
    QString mCopyModeName;
    QString mModeName;
};

class DeleteModeCommand: public Command
{
public:
    DeleteModeCommand(const QString& partName, const QString& modeName);
    void undo();
    void redo();

private:
    QString mPartName;
    QString mModeName;
    Part::Mode mModeCopy;
};

class ResetModeCommand: public Command
{
public:
    ResetModeCommand(const QString& partName, const QString& modeName);
    void undo();
    void redo();

private:
    QString mPartName;
    QString mModeName;
    Part::Mode mModeCopy;
};

class CopyModeCommand: public Command
{
public:
    CopyModeCommand(const QString& partName, const QString& modeName);
    void undo();
    void redo();

private:
    QString mPartName;
    QString mModeName;
    QString mNewModeName;
};

class RenameModeCommand: public Command
{
public:
    RenameModeCommand(const QString& partName, const QString& oldModeName, const QString& newModeName);
    void undo();
    void redo();

private:
    QString mPartName;
    QString mOldModeName;
    QString mNewModeName;
};

class NewCompositeCommand: public Command
{
public:
    NewCompositeCommand();
    void undo();
    void redo();

private:
    QString mName;
};

class CopyCompositeCommand: public Command
{
public:
    CopyCompositeCommand(const QString& name);
    void undo();
    void redo();

private:
    QString mName;
    QString mNewCompositeName;
};

class DeleteCompositeCommand: public Command {
public:
    DeleteCompositeCommand(QString name);
    ~DeleteCompositeCommand();
    void undo();
    void redo();

private:
    QString mName;
    Composite* mCopy;
};

class RenameCompositeCommand: public Command {
public:
    RenameCompositeCommand(QString oldName, QString newName);
    void undo();
    void redo();

private:
    QString mOldName, mNewName;
};















class DrawOnPartCommand: public Command {
public:
    DrawOnPartCommand(QString partName, QString mode, int frame, QImage data, QPoint offset);
    void undo();
    void redo();
private:
    QString mPartName;
    QString mMode;
    int mFrame;
    QImage mData;
    QPoint mOffset;
    QImage mOldFrame;
};

class EraseOnPartCommand: public Command {
public:
    EraseOnPartCommand(QString partName, QString mode, int frame, QImage data, QPoint offset);
    void undo();
    void redo();
private:
    QString mPartName;
    QString mMode;
    int mFrame;
    QImage mData;
    QPoint mOffset;
    QImage mOldFrame;
};

class NewFrameCommand: public Command {
public:
    NewFrameCommand(QString partName, QString modeName, int index);
    void undo();
    void redo();

private:
    QString mPartName, mModeName;
    int mIndex;
};

class CopyFrameCommand: public Command {
public:
    CopyFrameCommand(QString partName, QString modeName, int index);
    void undo();
    void redo();

private:
    QString mPartName, mModeName;
    int mIndex;
};

class DeleteFrameCommand: public Command {
public:
    DeleteFrameCommand(QString partName, QString modeName, int index);
    void undo();
    void redo();

private:
    QString mPartName, mModeName;
    int mIndex;
    QImage mImage;
    QPoint mAnchor;
    QPoint mPivots[MAX_PIVOTS];
};

class UpdateAnchorAndPivotsCommand: public Command {
public:
    UpdateAnchorAndPivotsCommand(QString partName, QString modeName, int index, QPoint anchor, QPoint p1, QPoint p2, QPoint p3, QPoint p4);
    void undo();
    void redo();

private:
    QString mPartName, mModeName;
    int mIndex;
    QPoint mAnchor, mPivots[MAX_PIVOTS];
    QPoint mOldAnchor, mOldPivots[MAX_PIVOTS];
};

class ChangeNumPivotsCommand: public Command {
public:
    ChangeNumPivotsCommand(QString partName, QString modeName, int numPivots);
    void undo();
    void redo();

private:
    QString mPartName, mModeName;
    int mIndex;
    int mNumPivots;
    int mOldNumPivots;
};

class ChangeModeSizeCommand: public Command {
public:
    ChangeModeSizeCommand(QString partName, QString modeName, int width, int height, int offsetX, int offsetY);
    void undo();
    void redo();

private:
    QString mPartName, mModeName;
    int mOldWidth, mOldHeight;
    int mWidth, mHeight, mOffsetX, mOffsetY;
    Part::Mode mOldMode;
};

class ChangeModeFPSCommand: public Command {
public:
    ChangeModeFPSCommand(QString partName, QString modeName, int fps);
    void undo();
    void redo();

private:
    QString mPartName, mModeName;
    int mOldFPS;
    int mFPS;
};

class EditCompositeChildCommand: public Command {
public:
    EditCompositeChildCommand(const QString& compName, const QString& childName, const QString& newPartName, int newZ, int newParent, int newParentPivot);
    void undo();
    void redo();

private:
    QString mCompName, mChildName;
    QString mOldPartName, mNewPartName;    
    int mOldParent, mNewParent;
    int mOldParentPivot, mNewParentPivot;
    int mOldRoot;
    int mOldZ, mNewZ;
};

class NewCompositeChildCommand: public Command {
public:
    NewCompositeChildCommand(const QString& compName);
    void redo();
    void undo();

private:
    QString mCompName, mChildName;
};

class EditCompositeChildNameCommand: public Command {
public:
    EditCompositeChildNameCommand(const QString& compName, const QString& child, const QString& newChildName);
    void redo();
    void undo();

private:
    QString mCompName, mOldChildName, mNewChildName;
};

class DeleteCompositeChildCommand: public Command {
public:
    DeleteCompositeChildCommand(const QString& compName, const QString& childName);
    void redo();
    void undo();

private:
    QString mCompName, mChildName;
    int mChildIndex;
    Composite mCompCopy;
};

class ChangePropertiesCommand: public Command {
public:
    ChangePropertiesCommand(QString partName, QString properties);
    void undo();
    void redo();

private:
    QString mPartName;
    QString mProperties;
    QString mOldProperties;
};

class ChangeCompPropertiesCommand: public Command {
public:
    ChangeCompPropertiesCommand(QString compName, QString properties);
    void undo();
    void redo();

private:
    QString mCompName;
    QString mProperties;
    QString mOldProperties;
};

#endif // COMMANDS_H
