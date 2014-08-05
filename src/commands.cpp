#include "commands.h"
#include "mainwindow.h"
#include <QObject>
#include <QString>

static int sNewCompositeSuffix = 0;
static int sNewModeSuffix = 0;

bool TryCommand(Command* command){
    if (command->ok){
        MainWindow::Instance()->undoStack()->push(command);
        return true;
    }
    else {
        delete command;
        return false;
    }
}

CNewPart::CNewPart() {
    mUuid = PM()->createAssetRef();
    mUuid.type = AssetType::Part;
    ok = true;
}

void CNewPart::undo()
{
    PM()->parts.take(mUuid);
    MainWindow::Instance()->partListChanged();
}

void CNewPart::redo()
{
    // Find a unique name
    QString name;
    int number = 0;
    do {
        name = (number==0)?QString("part"):(QObject::tr("part_") + QString::number(number));
        number++;
    } while (PM()->findPartByName(name)!=nullptr);

    QSharedPointer<Part> part =  QSharedPointer<Part>::create();
    part->ref = mUuid;
    part->name = name;
    Part::Mode mode;
    mode.numLayers = 1;
    mode.numFrames = 1;
    mode.numPivots = 0;
    mode.width = 16;
    mode.height = 16;
    mode.framesPerSecond = 12;
    mode.anchor.push_back(QPoint(mode.width/2,mode.height-1));
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].push_back(QPoint(0,0));
    }

    // Create blank frame
    // mode.images.push_back(img);

    {
        Part::Layer* layer = new Part::Layer;
        layer->name = "layer";
        layer->visible = true;
        auto img = QSharedPointer<QImage>::create(mode.width, mode.width, QImage::Format_ARGB32);
        img->fill(0x00FFFFFF);
        layer->frames.push_back(img);
        mode.layers.push_back(QSharedPointer<Part::Layer>(layer));
    }

    {
        Part::Layer* layer = new Part::Layer;
        layer->name = "layer 2";
        layer->visible = false;
        auto img = QSharedPointer<QImage>::create(mode.width, mode.width, QImage::Format_ARGB32);
        img->fill(0x00FFFFFF);
        layer->frames.push_back(img);
        mode.layers.push_back(QSharedPointer<Part::Layer>(layer));
    }


    part->modes.insert("mode", mode);
    PM()->parts.insert(part->ref, part);

    MainWindow::Instance()->partListChanged();
    MainWindow::Instance()->openPartWidget(part->ref);
}

CCopyPart::CCopyPart(AssetRef ref){
    mOriginal = ref;
    ok = PM()->hasPart(ref);
    if (ok){
        Part* part = PM()->getPart(ref);
        int copyNumber = 0;
        do {
            mNewPartName = part->name + "_" + QString::number(copyNumber++);
        } while (PM()->findPartByName(mNewPartName)!=nullptr);
        mCopy = PM()->createAssetRef();
        mCopy.type = AssetType::Part;
    }
}

void CCopyPart::undo(){
    PM()->parts.take(mCopy);
    MainWindow::Instance()->partListChanged();
}

void CCopyPart::redo(){
    auto partToCopy = PM()->parts.value(mOriginal);
    Q_ASSERT(partToCopy);

    QSharedPointer<Part> part = QSharedPointer<Part>::create();
    part->ref = mCopy;
    part->name = mNewPartName;
    part->parent = partToCopy->parent;
    part->properties = partToCopy->properties;
    QMapIterator<QString, Part::Mode> it(partToCopy->modes);
    while (it.hasNext()) {
        it.next();
        const QString& key = it.key();
        const Part::Mode& mode = it.value();
        Part::Mode newMode = mode;
        newMode.anchor = mode.anchor;
        for(int p=0;p<MAX_PIVOTS;p++)
            newMode.pivots[p] = mode.pivots[p];

         // make copy of layers and things
        newMode.numLayers = mode.numLayers;
        newMode.numFrames = mode.numFrames;

        for(auto oldLayer: mode.layers){
            QSharedPointer<Part::Layer> layer =  QSharedPointer<Part::Layer>::create();
            layer->name = oldLayer->name;
            layer->visible = oldLayer->visible;

            for(auto oldImage: oldLayer->frames){
                QSharedPointer<QImage> img = QSharedPointer<QImage>::create(*oldImage);
                layer->frames.push_back(img);
            }
            newMode.layers.push_back(layer);
        }
        part->modes.insert(key, newMode);
    }
    PM()->parts.insert(mCopy, part);

    // NB: Called from partlist
    // MainWindow::Instance()->partListChanged();
}

CDeletePart::CDeletePart(AssetRef ref):mRef(ref),mCopy() {
    ok = PM()->parts.contains(ref);
}

void CDeletePart::undo()
{
    PM()->parts.insert(mRef, mCopy);
    MainWindow::Instance()->partListChanged();
}

void CDeletePart::redo()
{
    mCopy = PM()->parts.take(mRef);

    // MainWindow::Instance()->partListChanged();
}

CRenamePart::CRenamePart(AssetRef ref, QString newName):mRef(ref){
    ok = PM()->parts.contains(ref);

    // Find new name with newName as base
    int num = 0;
    do {
        mNewName = (num==0)?newName:(newName + "_" + QString::number(num));
        num++;
    } while (PM()->findPartByName(mNewName)!=nullptr);
}

void CRenamePart::undo(){
    auto p = PM()->parts[mRef];
    p->name = mOldName;

    MainWindow::Instance()->partRenamed(mRef, mOldName);
}

void CRenamePart::redo(){
    auto p = PM()->parts[mRef];
    mOldName = p->name;
    p->name = mNewName;

    MainWindow::Instance()->partRenamed(mRef, mNewName);
}

CNewComposite::CNewComposite() {
    // Find a name
    int compSuffix = 0;
    do {
        mName = (compSuffix==0)?QString("comp"):(QObject::tr("comp_") + QString::number(compSuffix));
        compSuffix++;
    }
    while (PM()->findCompositeByName(mName)!=nullptr);
    mRef = PM()->createAssetRef();
    mRef.type = AssetType::Composite;
    ok = true;
}

void CNewComposite::undo()
{
    PM()->composites.take(mRef);
    MainWindow::Instance()->partListChanged();
}

void CNewComposite::redo()
{
    QSharedPointer<Composite> comp = QSharedPointer<Composite>::create();
    comp->root = -1;
    comp->name = mName;
    comp->ref = mRef;
    PM()->composites.insert(comp->ref, comp);
    MainWindow::Instance()->partListChanged();
    MainWindow::Instance()->openCompositeWidget(mRef);
}

CCopyComposite::CCopyComposite(AssetRef ref){
    Composite* comp = PM()->getComposite(ref);
    ok = (comp!=nullptr);
    if (ok){
        mOriginal = ref;
        int copyNumber = 0;
        do {
            mNewCompositeName = comp->name + "_" + QString::number(copyNumber++);
        }
        while (PM()->findCompositeByName(mNewCompositeName)!=nullptr);
    }
}

void CCopyComposite::undo(){
    PM()->composites.take(mCopy);
    MainWindow::Instance()->partListChanged();
}

void CCopyComposite::redo(){
    auto comp = PM()->getComposite(mOriginal);

    QSharedPointer<Composite> copy = QSharedPointer<Composite>::create();
    mCopy = PM()->createAssetRef();
    mCopy.type = AssetType::Composite;
    copy->ref = mCopy;
    copy->name = mNewCompositeName;
    copy->parent = comp->parent;
    copy->root = comp->root;
    copy->properties = comp->properties;
    copy->children = comp->children;
    copy->childrenMap = comp->childrenMap;
    PM()->composites.insert(copy->ref, copy);

    // MainWindow::Instance()->partListChanged();
}

CDeleteComposite::CDeleteComposite(AssetRef ref): mRef(ref), mCopy(){
    ok = PM()->hasComposite(ref);
}

void CDeleteComposite::undo()
{
    PM()->composites.insert(mRef, mCopy);
    MainWindow::Instance()->partListChanged();
}

void CDeleteComposite::redo()
{
    mCopy = PM()->composites.take(mRef);

    // MainWindow::Instance()->partListChanged();
}

CRenameComposite::CRenameComposite(AssetRef ref, QString newName):mRef(ref),mNewName(newName){
    Composite* comp = PM()->getComposite(ref);
    ok = comp!=nullptr;
    if (ok){
        mOldName = comp->name;
        int copyNumber = 0;
        do {
            mNewName = (copyNumber==0)?newName:(newName + "_" + QString::number(copyNumber));
            copyNumber++;
        }
        while (PM()->findCompositeByName(mNewName)!=nullptr);
    }
}

void CRenameComposite::undo(){
    Composite* p = PM()->getComposite(mRef);
    p->name = mOldName;

    MainWindow::Instance()->compositeRenamed(mRef, mOldName);
}

void CRenameComposite::redo(){
    Composite* p = PM()->getComposite(mRef);
    p->name = mNewName;

    MainWindow::Instance()->compositeRenamed(mRef, mNewName);
}

CNewFolder::CNewFolder() {
    mRef = PM()->createAssetRef();
    mRef.type = AssetType::Folder;
    ok = true;
}

void CNewFolder::undo()
{
    PM()->folders.take(mRef);
    MainWindow::Instance()->partListChanged();
}

void CNewFolder::redo()
{
    // Find a unique name
    QString name;
    int number = 0;
    do {
        name = (number==0)?QString("folder"):(QObject::tr("folder_") + QString::number(number));
        number++;
    } while (PM()->findFolderByName(name)!=nullptr);

    auto folder = QSharedPointer<Folder>::create();
    folder->ref = mRef;
    folder->name = name;
    PM()->folders.insert(folder->ref, folder);

    MainWindow::Instance()->partListChanged();
}

CDeleteFolder::CDeleteFolder(AssetRef ref):mRef(ref),mCopy() {
    ok = PM()->hasFolder(ref);
}

void CDeleteFolder::undo()
{
    qDebug() << "TODO: Undelete the folder contents";
    PM()->folders.insert(mRef, mCopy);

    MainWindow::Instance()->partListChanged();
}

void CDeleteFolder::redo()
{
    qDebug() << "TODO: Deleting the folder contents";
    mCopy = PM()->folders.take(mRef);

    // MainWindow::Instance()->partListChanged();
}

CRenameFolder::CRenameFolder(AssetRef ref, QString newName):mRef(ref){
    ok = PM()->folders.contains(ref);

    // Find new name with newName as base
    int num = 0;
    do {
        mNewName = (num==0)?newName:(newName + "_" + QString::number(num));
        num++;
    } while (PM()->findFolderByName(mNewName)!=nullptr);
}

void CRenameFolder::undo(){
    Folder* f = PM()->getFolder(mRef);
    f->name = mOldName;

    MainWindow::Instance()->folderRenamed(mRef, mOldName);
}

void CRenameFolder::redo(){
    Folder* f = PM()->getFolder(mRef);
    mOldName = f->name;
    f->name = mNewName;

    MainWindow::Instance()->folderRenamed(mRef, mNewName);
}



CMoveAsset::CMoveAsset(AssetRef ref, AssetRef newParent):mRef(ref),mNewParent(newParent){
    ok = PM()->hasAsset(ref);
    ok = ok && (newParent.isNull() || PM()->hasFolder(newParent));
    if (ok){
        mOldParent = PM()->getAsset(ref)->parent;
    }
}

void CMoveAsset::undo(){
    // Move the asset back
    Asset* asset = PM()->getAsset(mRef);
    asset->parent = mOldParent;

    Folder* newParent = PM()->getFolder(mNewParent);
    if (newParent){
        newParent->children.removeAll(asset->parent);
    }

    Folder* oldParent = PM()->getFolder(mOldParent);
    if (oldParent){
        oldParent->children.append(asset->ref);
    }

    MainWindow::Instance()->partListChanged();
}

void CMoveAsset::redo(){
    // Move the asset
    Asset* asset = PM()->getAsset(mRef);
    asset->parent = mNewParent;

    Folder* newParent = PM()->getFolder(mNewParent);
    if (newParent){
        newParent->children.append(asset->parent);
    }

    Folder* oldParent = PM()->getFolder(mOldParent);
    if (oldParent){
        oldParent->children.removeAll(asset->ref);
    }

    // NB: partListChanged() is called just once from PartList after all its moves are done
    // MainWindow::Instance()->partListChanged();
}














CNewMode::CNewMode(AssetRef part, const QString& copyModeName):mPart(part), mCopyModeName(copyModeName){
    ok = PM()->hasPart(mPart) && PM()->getPart(mPart)->modes.contains(mCopyModeName);

    if (ok){
        // Find a name
        int suffix = 0;
        auto p = PM()->parts.value(mPart);
        QStringList modeList = p->modes.keys();
        do {
            mModeName = QString("m") + QString("%1").arg(suffix, 3, 10, QChar('0'));
            suffix++;

        } while (modeList.contains(mModeName));
    }
}

void CNewMode::undo(){
    // remove the mode..
    auto p = PM()->getPart(mPart);
    p->modes.take(mModeName);
    MainWindow::Instance()->partModesChanged(mPart);
}

void CNewMode::redo(){
    auto p = PM()->parts.value(mPart);
    Part::Mode copyMode = p->modes.value(mCopyModeName);
    Part::Mode m;
    m.numFrames = 1;
    m.width = copyMode.width;
    m.height = copyMode.height;
    m.numPivots = copyMode.numPivots;
    m.numLayers = 1;
    m.framesPerSecond = copyMode.framesPerSecond;

    auto layer = QSharedPointer<Part::Layer>::create();
    layer->name = "layer";
    layer->visible = true;

    auto img = QSharedPointer<QImage>::create(m.width, m.width, QImage::Format_ARGB32);
    img->fill(0x00FFFFFF);
    layer->frames.push_back(img);
    m.layers.push_back(layer);

    /*
    for(int k=0;k<m.numFrames;k++){
        for(int p=0;p<MAX_PIVOTS;p++){
            m.pivots[p].push_back(QPoint(0,0));
        }
        m.anchor.push_back(QPoint(0,0));
        QImage* img = new QImage(m.width, m.height, QImage::Format_ARGB32);
        m.images.push_back(img);
        img->fill(QColor(255,255,255,0));
    }
    */
    p->modes.insert(mModeName,m);

    MainWindow::Instance()->partModesChanged(mPart);
}


CDeleteMode::CDeleteMode(AssetRef part, const QString& modeName):mPart(part),mModeName(modeName),mModeCopy(){
    // mModeCopy
    ok = PM()->hasPart(mPart) && PM()->getPart(mPart)->modes.contains(mModeName);
}

void CDeleteMode::undo(){
    // re-add the mode..
    auto p = PM()->getPart(mPart);
    p->modes.insert(mModeName, mModeCopy);
    MainWindow::Instance()->partModesChanged(mPart);
}

void CDeleteMode::redo(){
    // remove the mode..
    auto p = PM()->getPart(mPart);
    mModeCopy = p->modes.take(mModeName);
    MainWindow::Instance()->partModesChanged(mPart);
}


CResetMode::CResetMode(AssetRef part, const QString& modeName):mPart(part),mModeName(modeName){
    // mModeCopy
    ok = PM()->hasPart(mPart) && PM()->getPart(mPart)->modes.contains(mModeName);
}

void CResetMode::undo(){
    // re-add the mode..
    auto p = PM()->getPart(mPart);
    p->modes.remove(mModeName);
    p->modes.insert(mModeName, mModeCopy);

    MainWindow::Instance()->partModesChanged(mPart);
}

void CResetMode::redo(){
    // reset the mode..
    auto p = PM()->getPart(mPart);
    Part::Mode& mode = p->modes[mModeName];

    // Make a copy
    mModeCopy = mode;
    mModeCopy.anchor = mode.anchor;
    mModeCopy.layers = mode.layers;
    mModeCopy.numLayers = 1;
    for(int p=0;p<MAX_PIVOTS;p++){
        mModeCopy.pivots[p] = mode.pivots[p];
    }

    mode.layers.clear();
    mode.anchor.clear();
    for(int p=0;p<MAX_PIVOTS;p++){
        mode.pivots[p].clear();
    }

    // New layer
    auto layer = QSharedPointer<Part::Layer>::create();
    layer->name = "layer";
    layer->visible = true;

    auto newImage = QSharedPointer<QImage>::create(mode.width, mode.height, QImage::Format_ARGB32);
    newImage->fill(0x00FFFFFF);
    layer->frames.push_back(newImage);
    mode.layers.push_back(layer);

    mode.anchor.push_back(QPoint(0,0));
    for(int p=0;p<MAX_PIVOTS;p++){
        mode.pivots[p].push_back(QPoint(0,0));
    }
    mode.numPivots = 0;
    mode.numFrames = 1;
    mode.framesPerSecond = 24;

    MainWindow::Instance()->partModesChanged(mPart);
}


CCopyMode::CCopyMode(AssetRef part, const QString& modeName):mPart(part), mModeName(modeName){
    ok = PM()->hasPart(mPart) && PM()->getPart(mPart)->modes.contains(mModeName);

    if (ok){
        // Find a name
        int suffix = 0;
        auto p = PM()->parts.value(mPart);
        QStringList modeList = p->modes.keys();
        do {
            mNewModeName = QString("m") + QString("%1").arg(suffix, 3, 10, QChar('0'));
            suffix++;

        } while (modeList.contains(mNewModeName));
    }
}

void CCopyMode::undo(){
    // remove the mode..
    auto p = PM()->getPart(mPart);
    Part::Mode mode = p->modes.take(mNewModeName);
    MainWindow::Instance()->partModesChanged(mPart);
}

void CCopyMode::redo(){
    auto p = PM()->getPart(mPart);
    const Part::Mode& copyMode = p->modes.value(mModeName);
    Part::Mode m;
    m.numFrames = copyMode.numFrames;
    m.width = copyMode.width;
    m.height = copyMode.height;
    m.numPivots = copyMode.numPivots;
    m.framesPerSecond = copyMode.framesPerSecond;
    for(int p=0;p<MAX_PIVOTS;p++)
        m.pivots[p] = copyMode.pivots[p];
    m.anchor = copyMode.anchor;

    // make copy of layers and things
    m.numLayers = copyMode.numLayers;

    for(auto oldLayer: copyMode.layers){
        auto layer = QSharedPointer<Part::Layer>::create();
        layer->name = oldLayer->name;
        layer->visible = oldLayer->visible;

        for(auto oldImage: oldLayer->frames){
            auto img = QSharedPointer<QImage>::create(*oldImage);
            layer->frames.push_back(img);
        }
        m.layers.push_back(layer);
    }

    /*
    for(int k=0;k<m.numFrames;k++){
        QImage* img = new QImage(copyMode.images.at(k)->copy());
        m.images.push_back(img);
    }
    */

    p->modes.insert(mNewModeName,m);
    MainWindow::Instance()->partModesChanged(mPart);
}

CRenameMode::CRenameMode(AssetRef part, const QString& oldModeName, const QString& newModeName)
    :mPart(part), mOldModeName(oldModeName), mNewModeName(newModeName){
    mNewModeName = mNewModeName.trimmed(); // .simplified();
    mNewModeName.replace(' ','_');

    if (mNewModeName.size()==0){
        mNewModeName = "_";
    }

    ok =PM()->hasPart(mPart) && PM()->getPart(mPart)->modes.contains(mOldModeName) && !PM()->getPart(mPart)->modes.contains(mNewModeName);
}

void CRenameMode::undo(){
    Part* p = PM()->getPart(mPart);
    Part::Mode m = p->modes.take(mNewModeName);
    p->modes.insert(mOldModeName, m);

    MainWindow::Instance()->partModeRenamed(mPart, mNewModeName, mOldModeName);
}

void CRenameMode::redo(){
    Part* p = PM()->getPart(mPart);
    Part::Mode m = p->modes.take(mOldModeName);
    p->modes.insert(mNewModeName, m);

    MainWindow::Instance()->partModeRenamed(mPart, mOldModeName, mNewModeName);
}






CDrawOnPart::CDrawOnPart(AssetRef part, QString mode, int frame, QImage data, QPoint offset)
    :mPart(part),mMode(mode),mFrame(frame),mData(data),mOffset(offset){
    Part* p = PM()->getPart(mPart);
    ok = p &&
            p->modes.contains(mode) &&
            p->modes[mode].numFrames>=frame &&
            p->modes[mode].layers.at(0)->frames.at(frame)!=nullptr;
}

void CDrawOnPart::undo(){
    //qDebug() << "CDrawOnPart::undo()";
    // Reload the old frame
    auto img = PM()->getPart(mPart)->modes[mMode].layers.at(0)->frames.at(mFrame);
    QPainter painter(img.data());
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(0, 0, mOldFrame);
    // *img = mOldFrame;

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPart, mMode, mFrame);
}

void CDrawOnPart::redo(){
    //qDebug() << "CDrawOnPart::redo()";
    // Record the old frame
    // Draw the image into the part
    auto img = PM()->getPart(mPart)->modes[mMode].layers.at(0)->frames.at(mFrame);
    mOldFrame = img->copy();
    QPainter painter(img.data());
    painter.drawImage(mOffset.x(), mOffset.y(), mData);

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPart, mMode, mFrame);
}

CEraseOnPart::CEraseOnPart(AssetRef part, QString mode, int frame, QImage data, QPoint offset)
    :mPart(part),mMode(mode),mFrame(frame),mData(data),mOffset(offset){
    Part* p = PM()->getPart(part);
    ok = p &&
            p->modes.contains(mode) &&
            p->modes[mode].numFrames>=frame &&
            p->modes[mode].layers.at(0)->frames.at(frame)!=nullptr;
}

void CEraseOnPart::undo(){
    // Reload the old frame
    auto img = PM()->getPart(mPart)->modes[mMode].layers.at(0)->frames.at(mFrame);
    QPainter painter(img.data());
    painter.drawImage(0, 0, mOldFrame);
    // *img = mOldFrame;

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPart, mMode, mFrame);
}

void CEraseOnPart::redo(){
    // Record the old frame
    // Draw the image into the part
    auto img = PM()->getPart(mPart)->modes[mMode].layers.at(0)->frames.at(mFrame);
    mOldFrame = *img;
    QPainter painter(img.data());
    painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    painter.drawImage(mOffset.x(), mOffset.y(), mData);

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPart, mMode, mFrame);
}



CNewFrame::CNewFrame(AssetRef part, QString modeName, int index)
    :mPart(part), mModeName(modeName), mIndex(index){
    ok = PM()->hasPart(part) &&
            PM()->getPart(part)->modes.contains(modeName);
    if (ok){
        int numFrames = PM()->getPart(part)->modes[modeName].numFrames;
        ok = ok && (index>=0 && index<=numFrames);
    }
}

void CNewFrame::undo(){
    auto part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mode.layers.at(0)->frames.takeAt(mIndex);
    mode.anchor.removeAt(mIndex);
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].removeAt(mIndex);
    }
    mode.numFrames--;

    MainWindow::Instance()->partFramesUpdated(mPart, mModeName);
}

void CNewFrame::redo(){
    // Create the new frame
    auto part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    auto image = QSharedPointer<QImage>::create(mode.width, mode.height, QImage::Format_ARGB32);
    image->fill(0x00FFFFFF);
    mode.layers.at(0)->frames.insert(mIndex, image);

    if (mIndex<mode.numFrames)
        mode.anchor.insert(mIndex, mode.anchor.at(mIndex));
    else if (mode.numFrames>0)
        mode.anchor.insert(mIndex, mode.anchor.at(0));
    else
        mode.anchor.insert(mIndex, QPoint(0,0));

    for(int i=0;i<MAX_PIVOTS;i++){
        if (mIndex<mode.numFrames)
            mode.pivots[i].insert(mIndex, mode.pivots[i].at(mIndex));
        else if (mode.numFrames>0)
            mode.pivots[i].insert(mIndex, mode.pivots[i].at(0));
        else
            mode.pivots[i].insert(mIndex, QPoint(0,0));
    }
    mode.numFrames++;

    MainWindow::Instance()->partFramesUpdated(mPart, mModeName);
}

CCopyFrame::CCopyFrame(AssetRef part, QString modeName, int index)
    :mPart(part), mModeName(modeName), mIndex(index){
    ok = PM()->hasPart(part) &&
            PM()->getPart(part)->modes.contains(modeName);
    if (ok){
        int numFrames = PM()->getPart(mPart)->modes[modeName].numFrames;
        ok = ok && (index>=0 && index<numFrames);
    }
}

void CCopyFrame::undo(){
    auto part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mode.layers.at(0)->frames.takeAt(mIndex+1);
    mode.anchor.removeAt(mIndex+1);
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].removeAt(mIndex+1);
    }
    mode.numFrames--;
    MainWindow::Instance()->partFramesUpdated(mPart, mModeName);
}

void CCopyFrame::redo(){
    // Create the new frame
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];

    auto image = QSharedPointer<QImage>();
    if (mIndex<mode.numFrames){
        mode.anchor.insert(mIndex+1, mode.anchor.at(mIndex));
        image.reset(new QImage(mode.layers.at(0)->frames.at(mIndex)->copy()));
    }
    else if (mode.numFrames>0){
        mode.anchor.insert(mIndex+1, mode.anchor.at(0));
        image.reset(new QImage(mode.layers.at(0)->frames.at(0)->copy()));
    }
    else {
        mode.anchor.insert(mIndex+1, QPoint(0,0));
        image.reset(new QImage(mode.width, mode.height, QImage::Format_ARGB32));
        image->fill(0x00FFFFFF);
    }
    mode.layers.at(0)->frames.insert(mIndex+1, image);

    for(int i=0;i<MAX_PIVOTS;i++){
        if (mIndex<mode.numFrames)
            mode.pivots[i].insert(mIndex+1, mode.pivots[i].at(mIndex));
        else if (mode.numFrames>0)
            mode.pivots[i].insert(mIndex+1, mode.pivots[i].at(0));
        else
            mode.pivots[i].insert(mIndex+1, QPoint(0,0));
    }
    mode.numFrames++;

    MainWindow::Instance()->partFramesUpdated(mPart, mModeName);
}

CDeleteFrame::CDeleteFrame(AssetRef part, QString modeName, int index)
    :mPart(part), mModeName(modeName), mIndex(index){
    ok = PM()->hasPart(part) &&
            PM()->getPart(part)->modes.contains(modeName);
    if (ok){
        int numFrames = PM()->getPart(mPart)->modes[modeName].numFrames;
        ok = ok && (index>=0 && index<numFrames);
    }
}

void CDeleteFrame::undo(){
    // Create the new frame
    auto part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mode.layers.at(0)->frames.insert(mIndex, mImage);
    mode.anchor.insert(mIndex, mAnchor);
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].insert(mIndex, mPivots[i]);
    }
    mode.numFrames++;
    mImage.clear();

    MainWindow::Instance()->partFramesUpdated(mPart, mModeName);

}

void CDeleteFrame::redo(){
    // NB: Remember old frame info (image, etc..)
    auto part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mImage = mode.layers.at(0)->frames.takeAt(mIndex);
    mAnchor = mode.anchor.takeAt(mIndex);
    for(int i=0;i<MAX_PIVOTS;i++){
        mPivots[i] = mode.pivots[i].takeAt(mIndex);
    }
    mode.numFrames--;

    MainWindow::Instance()->partFramesUpdated(mPart, mModeName);
}





CUpdateAnchorAndPivots::CUpdateAnchorAndPivots(AssetRef part, QString modeName, int index, QPoint anchor, QPoint p1, QPoint p2, QPoint p3, QPoint p4)
    :mPart(part), mModeName(modeName), mIndex(index), mAnchor(anchor) {
    mPivots[0] = p1;
    mPivots[1] = p2;
    mPivots[2] = p3;
    mPivots[3] = p4;

    Part* p = PM()->getPart(mPart);
    ok = p &&
            p->modes.contains(mModeName) &&
            p->modes[mModeName].numFrames>mIndex;
}

void CUpdateAnchorAndPivots::undo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mode.anchor.replace(mIndex, mOldAnchor);
    for(int i=0;i<mode.numPivots;i++){
        mode.pivots[i].replace(mIndex, mOldPivots[i]);
    }
    MainWindow::Instance()->partFrameUpdated(mPart, mModeName, mIndex);
}

void CUpdateAnchorAndPivots::redo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mOldAnchor = mode.anchor.at(mIndex);
    mode.anchor.replace(mIndex, mAnchor);
    for(int i=0;i<mode.numPivots;i++){
        mOldPivots[i] = mode.pivots[i].at(mIndex);
        mode.pivots[i].replace(mIndex, mPivots[i]);
    }

    MainWindow::Instance()->partFrameUpdated(mPart, mModeName, mIndex);
}

CChangeNumPivots::CChangeNumPivots(AssetRef part, QString modeName, int numPivots)
    :mPart(part), mModeName(modeName), mNumPivots(numPivots){
    ok = PM()->hasPart(part) &&
            PM()->getPart(part)->modes.contains(modeName);
}

void CChangeNumPivots::undo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mode.numPivots = mOldNumPivots;

    MainWindow::Instance()->partNumPivotsUpdated(mPart, mModeName);
}

void CChangeNumPivots::redo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mOldNumPivots = mode.numPivots;
    mode.numPivots = mNumPivots;

    MainWindow::Instance()->partNumPivotsUpdated(mPart, mModeName);
}

CChangeModeSize::CChangeModeSize(AssetRef part, QString modeName, int width, int height, int offsetx, int offsety)
    :mPart(part), mModeName(modeName), mWidth(width), mHeight(height), mOffsetX(offsetx), mOffsetY(offsety){
    ok = PM()->hasPart(mPart) &&
            PM()->getPart(mPart)->modes.contains(modeName);
    ok = ok && mWidth>0 && mHeight>0;
}

void CChangeModeSize::undo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    part->modes[mModeName] = mOldMode;
    MainWindow::Instance()->partModesChanged(mPart);
}

void CChangeModeSize::redo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mOldWidth = mode.width;
    mOldHeight = mode.height;

    // make copy of old mode
    mOldMode = mode;
    mOldMode.layers.at(0)->frames.clear();
    for(int p=0;p<MAX_PIVOTS;p++)
        mOldMode.pivots[p] = mOldMode.pivots[p];
    for(int k=0;k<mOldMode.numFrames;k++){
        // QImage* img = new QImage(mode.images.at(k)->copy());
        mOldMode.layers.at(0)->frames.push_back(mode.layers.at(0)->frames.at(k)); // this ref will be removed below anyway
    }

    // Modify mode to new dimensions
    mode.width = mWidth;
    mode.height = mHeight;
    for(int k=0;k<mode.numFrames;k++){
        auto newImage = QSharedPointer<QImage>::create(mWidth, mHeight, QImage::Format_ARGB32);
        newImage->fill(0x00FFFFFF);
        QPainter painter(newImage.data());
        painter.drawImage(mOffsetX,mOffsetY,*mode.layers.at(0)->frames.at(k));
        painter.end();
        mode.layers.at(0)->frames.replace(k, newImage);

        mode.anchor[k] += QPoint(mOffsetX,mOffsetY);
        for(int p=0;p<mode.numPivots;p++){
            mode.pivots[p][k] += QPoint(mOffsetX,mOffsetY);
        }
    }

    // mOldWidth, mOldHeight
    // save mOldMode

    MainWindow::Instance()->partModesChanged(mPart);
}

CChangeModeFPS::CChangeModeFPS(AssetRef part, QString modeName, int fps)
    :mPart(part), mModeName(modeName), mFPS(fps){
    ok = PM()->hasPart(mPart) &&
            PM()->getPart(mPart)->modes.contains(modeName);
    ok = ok && mFPS>0;
}

void CChangeModeFPS::undo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mode.framesPerSecond = mOldFPS;

    MainWindow::Instance()->partModesChanged(mPart);
}

void CChangeModeFPS::redo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mOldFPS = mode.framesPerSecond;
    mode.framesPerSecond = mFPS;

    MainWindow::Instance()->partModesChanged(mPart);
}








CEditCompositeChild::CEditCompositeChild(AssetRef comp, const QString& childName, AssetRef newPart, int newZ, int newParent, int newParentPivot)
    :mComp(comp), mChildName(childName), mNewPart(newPart), mNewParent(newParent), mNewParentPivot(newParentPivot), mNewZ(newZ)
{
    ok = PM()->hasComposite(mComp) && PM()->getComposite(mComp)->childrenMap.contains(mChildName);
}

void CEditCompositeChild::undo(){
    Composite* comp = PM()->getComposite(mComp);
    Composite::Child& child = comp->childrenMap[mChildName];

    child.parent = mOldParent;
    child.parentPivot = mOldParentPivot;

    child.part = mOldPart;
    child.z = mOldZ;

    // qDebug() << "CEditCompositeChild: Updating child: " << mChildName << child.part << mNewZ << child.parent << child.parentPivot;

    // Fix all parent/children links...
    if (mOldParent!=mNewParent){
        // remove child from new parent
        if (mNewParent>=0 && mNewParent<comp->children.count()){
            Composite::Child& parent = comp->childrenMap[comp->children[mNewParent]];
            int index = parent.children.indexOf(child.index);
            if (index>=0){
                // qDebug() << "CEditCompositeChild: Removing child from " << comp->children[mOldParent];
                parent.children.removeAt(index);
            }
        }

        // add child back to new parent
        if (mOldParent!=child.index && mOldParent>=0 && mOldParent<comp->children.count()){
            Composite::Child& parent = comp->childrenMap[comp->children[mOldParent]];
            int index = parent.children.indexOf(child.index);
            if (index==-1){
                // qDebug() << "CEditCompositeChild: Adding child to " << comp->children[mNewParent];
                parent.children.append(child.index);
            }
        }
    }

    // Restore root
    comp->root = mOldRoot;

    // Update comp..
    MainWindow::Instance()->compositeUpdatedMinorChanges(mComp);
}

void CEditCompositeChild::redo(){
    Composite* comp = PM()->getComposite(mComp);
    Composite::Child& child = comp->childrenMap[mChildName];

    mOldPart = child.part;
    mOldZ = child.z;
    mOldParent = child.parent;
    mOldParentPivot = child.parentPivot;

    child.parent = std::max(-1, mNewParent);
    child.parentPivot = std::max(-1, mNewParentPivot);

    // Part* newPart = PM()->getPart(mNewPart);
    child.part = mNewPart; // newPart?newPart->ref:AssetRef();
    child.z = mNewZ;

    // Fix all parent/children links...
    if (mOldParent!=mNewParent){
        // remove child from old parent
        if (mOldParent>=0 && mOldParent<comp->children.count()){
            Composite::Child& parent = comp->childrenMap[comp->children[mOldParent]];
            int index = parent.children.indexOf(child.index);
            if (index>=0){
                // qDebug() << "CEditCompositeChild: Removing child from " << comp->children[mOldParent];
                parent.children.removeAt(index);
            }
        }

        // add child to new parent
        // NB: cycles and bad relationships are just ignored in the comp widget (but allowed in the spec)
        if (mNewParent!=child.index && mNewParent>=0 && mNewParent<comp->children.count()){
            Composite::Child& parent = comp->childrenMap[comp->children[mNewParent]];
            int index = parent.children.indexOf(child.index);
            if (index==-1){
                // qDebug() << "CEditCompositeChild: Adding child to " << comp->children[mNewParent];
                parent.children.append(child.index);
            }
        }
    }

    // Update root to point to first -1
    mOldRoot = comp->root;
    comp->root = 0;
    for(int i=0;i<comp->children.count();i++){
        if (comp->childrenMap[comp->children[i]].parent==-1){
            comp->root = i;
        }
    }

    // Update comp..
    MainWindow::Instance()->compositeUpdatedMinorChanges(mComp);
}

CNewCompositeChild::CNewCompositeChild(AssetRef comp):
    mComp(comp){
    ok = PM()->hasComposite(mComp);

    if (ok){
        Composite* comp = PM()->getComposite(mComp);

        // Find a unique name..
        int suffix = 0;
        do {
            mChildName = QString("c") + QString("%1").arg(suffix, 3, 10, QChar('0'));
            suffix++;
        }
        while (comp->children.contains(mChildName));
    }
}

void CNewCompositeChild::undo(){
    // Delete the child..
    Composite* comp = PM()->getComposite(mComp);
    comp->children.removeAll(mChildName);
    comp->childrenMap.remove(mChildName);

    // Update widgets
    MainWindow::Instance()->compositeUpdated(mComp);
}

void CNewCompositeChild::redo(){
    // Add a new blank child..
    Composite* comp = PM()->getComposite(mComp);
    comp->children.append(mChildName);

    Composite::Child child;
    child.index = comp->children.count()-1;
    child.parent = -1;
    child.parentPivot = -1;
    child.part = AssetRef(); // blank
    child.z = 0;
    comp->childrenMap.insert(mChildName, child);

    // Update widgets
    MainWindow::Instance()->compositeUpdated(mComp);
}

CEditCompositeChildName::CEditCompositeChildName(AssetRef ref, const QString& child, const QString& newChildName)
    :mComp(ref), mOldChildName(child), mNewChildName(newChildName){
    Composite* comp = PM()->getComposite(mComp);
    ok = comp && comp->children.contains(child);
    if (ok){
        // Find a unique child name
        int childIndex = 0;
        while (comp->children.contains(mNewChildName)){
            mNewChildName = newChildName + QString::number(childIndex++);
        }
    }
}

void CEditCompositeChildName::undo(){
    // Unchange the child name
    Composite* comp = PM()->getComposite(mComp);
    comp->children.replace(comp->children.indexOf(mNewChildName), mOldChildName);
    comp->childrenMap.insert(mOldChildName, comp->childrenMap.take(mNewChildName));
    MainWindow::Instance()->compositeUpdated(mComp);
}

void CEditCompositeChildName::redo(){
    // Change the child name
    Composite* comp = PM()->getComposite(mComp);
    comp->children.replace(comp->children.indexOf(mOldChildName), mNewChildName);
    comp->childrenMap.insert(mNewChildName, comp->childrenMap.take(mOldChildName));
    MainWindow::Instance()->compositeUpdated(mComp);
}

CDeleteCompositeChild::CDeleteCompositeChild(AssetRef ref, const QString& childName)
    :mComp(ref), mChildName(childName), mCompCopy(){

    auto comp = PM()->getComposite(mComp);
    ok = comp && comp->children.contains(childName) && comp->childrenMap.contains(childName);
    if (ok){
        mChildIndex = comp->children.indexOf(childName);
        ok = (mChildIndex>=0);
    }
}

void CDeleteCompositeChild::undo(){
    // Overwrite the old comp
    PM()->composites.insert(mComp, mCompCopy);
    mCompCopy.clear();
    MainWindow::Instance()->compositeUpdated(mComp);
}

int FixIndex(int i, int ci){
    if (i==ci) return -1;
    else if (i>ci) return i-1;
    else return i;
}

void CDeleteCompositeChild::redo(){
    auto comp = PM()->getComposite(mComp);
    mCompCopy = QSharedPointer<Composite>::create(*comp); // make a copy so we can undo it

    // We need to adjust all the index references
    // Let ci = mChildIndex
    int ci = mChildIndex;

    // Any ref i=ci -> i=-1
    // Any ref i>ci -> i=ci-1
    QMutableMapIterator<QString, Composite::Child> it(comp->childrenMap);
    while (it.hasNext()){
        it.next();
        Composite::Child& child = it.value();
        if (child.index==mChildIndex) continue;

        child.index = FixIndex(child.index, ci);
        child.parent = FixIndex(child.parent, ci);

        QMutableListIterator<int> lit(child.children);
        while (lit.hasNext()){
            lit.next();
            lit.value() = FixIndex(lit.value(), ci);
            if (lit.value()==-1){
                lit.remove();
            }
        }
    }

    comp->childrenMap.remove(mChildName);
    comp->children.removeOne(mChildName);

    // Update root if necessary
    if (comp->root==mChildIndex){
        comp->root = -1;
        foreach(const QString& cn, comp->children){
            const Composite::Child& child = comp->childrenMap.value(cn);
            if (child.parent==-1){
                comp->root = child.index;
                break;
            }
        }
    }
    else if (comp->root>mChildIndex){
        comp->root--;
    }

    MainWindow::Instance()->compositeUpdated(mComp);
}


CChangePartProperties::CChangePartProperties(AssetRef part, QString properties)
    :mPart(part), mProperties(properties){
    ok = PM()->hasPart(mPart);
}

void CChangePartProperties::undo(){
    Part* part = PM()->getPart(mPart);
    part->properties = mOldProperties;
    MainWindow::Instance()->partPropertiesUpdated(mPart);
}

void CChangePartProperties::redo(){
    Part* part = PM()->getPart(mPart);
    mOldProperties = part->properties;
    part->properties = mProperties;

    MainWindow::Instance()->partPropertiesUpdated(mPart);
}

CChangeCompProperties::CChangeCompProperties(AssetRef comp, QString properties)
    :mComp(comp), mProperties(properties){
    ok = PM()->hasComposite(mComp);
}

void CChangeCompProperties::undo(){
    Composite* comp = PM()->getComposite(mComp);
    comp->properties = mOldProperties;

    MainWindow::Instance()->compPropertiesUpdated(mComp);
}

void CChangeCompProperties::redo(){
    Composite* comp = PM()->getComposite(mComp);
    mOldProperties = comp->properties;
    comp->properties = mProperties;

    MainWindow::Instance()->compPropertiesUpdated(mComp);
}

