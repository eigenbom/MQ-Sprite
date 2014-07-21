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

NewPartCommand::NewPartCommand() {    
    mUuid = PM()->createAssetRef();
    ok = true;
}

void NewPartCommand::undo()
{
    Part* p = PM()->parts.take(mUuid);
    delete p;
    MainWindow::Instance()->partListChanged();
}

void NewPartCommand::redo()
{
    // Find a unique name
    QString name;
    int number = 0;
    do {
      name = (number==0)?QString("part"):(QObject::tr("part_") + QString::number(number));
      number++;
    } while (PM()->findPartByName(name)!=nullptr);

    Part* part = new Part;
    part->ref = PM()->createAssetRef();
    part->name = name;
    Part::Mode mode;
    mode.numFrames = 1;
    mode.numPivots = 0;
    mode.width = 8;
    mode.height = 8;
    mode.framesPerSecond = 12;
    mode.anchor.push_back(QPoint(4,4));
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].push_back(QPoint(0,0));
    }
    QImage* img = new QImage(mode.width, mode.width, QImage::Format_ARGB32);
    img->fill(0x00FFFFFF);
    mode.images.push_back(img);
    part->modes.insert("m000", mode);
    PM()->parts.insert(part->ref, part);

    MainWindow::Instance()->partListChanged();
    MainWindow::Instance()->openPartWidget(part->ref);

}

CopyPartCommand::CopyPartCommand(AssetRef ref){
    mOriginal = ref;
    ok = PM()->hasPart(ref);
    if (ok){
        Part* part = PM()->getPart(ref);
        int copyNumber = 0;
        do {
            mNewPartName = part->name + "_" + QString::number(copyNumber++);
        } while (PM()->findPartByName(mNewPartName)!=nullptr);
        mCopy = PM()->createAssetRef();
    }
}

void CopyPartCommand::undo(){
    Part* p = PM()->parts.take(mCopy);
    delete p;
    // NB: images are deleted in the parts destructor

    MainWindow::Instance()->partListChanged();
}

void CopyPartCommand::redo(){
    Part* part = new Part;
    part->ref = mCopy;
    part->name = mNewPartName;

    const Part* partToCopy = PM()->parts.value(mOriginal);
    Q_ASSERT(partToCopy);
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
        // make copy of images..
        for(int f=0;f<mode.numFrames;f++){
            newMode.images.replace(f, new QImage(newMode.images.at(f)->copy()));
        }
        part->modes.insert(key, newMode);
    }
    PM()->parts.insert(mCopy, part);

    MainWindow::Instance()->partListChanged();
}

DeletePartCommand::DeletePartCommand(AssetRef ref):mRef(ref),mCopy(nullptr) {
    ok = PM()->parts.contains(ref);
}

DeletePartCommand::~DeletePartCommand(){
    delete mCopy;
}

void DeletePartCommand::undo()
{
    PM()->parts.insert(mRef, mCopy);
    mCopy = nullptr;
    MainWindow::Instance()->partListChanged();
}

void DeletePartCommand::redo()
{
    mCopy = PM()->parts[mRef];
    PM()->parts.remove(mRef);
    MainWindow::Instance()->partListChanged();    
}

RenamePartCommand::RenamePartCommand(AssetRef ref, QString newName):mRef(ref){
    ok = PM()->parts.contains(ref);

    // Find new name with newName as base
    int num = 0;
    do {
        mNewName = (num==0)?newName:(newName + "_" + QString::number(num));
        num++;
    } while (PM()->findPartByName(mNewName)!=nullptr);
}

void RenamePartCommand::undo(){
    // Part* p = PM()->parts[mNewName];
    // PM()->parts.remove(mNewName);
    // PM()->parts.insert(mOldName, p);

    Part* p = PM()->parts[mRef];
    p->name = mOldName;

    // update comps too...
    /*
    foreach(Composite* comp, PM()->composites.values()){        
        QMutableMapIterator<QString,Composite::Child> it(comp->childrenMap);
        while (it.hasNext()){
            it.next();
            Composite::Child& ch = it.value();
            if (ch.part == mNewName){
                ch.part = mOldName;
            }
        }
    }*/

    MainWindow::Instance()->partRenamed(mRef, mOldName);
}

void RenamePartCommand::redo(){

    // Part* p = PM()->parts[mOldName];
    // PM()->parts.remove(mOldName);
    // PM()->parts.insert(mNewName, p);
    Part* p = PM()->parts[mRef];
    mOldName = p->name;
    p->name = mNewName;

    // update comps too...
    /*
    foreach(Composite* comp, PM()->composites.values()){
        QMutableMapIterator<QString,Composite::Child> it(comp->childrenMap);
        while (it.hasNext()){
            it.next();
            Composite::Child& ch = it.value();
            if (ch.part==mOldName){
                ch.part = mNewName;
            }
        }
    }*/

    MainWindow::Instance()->partRenamed(mRef, mNewName);
}

NewCompositeCommand::NewCompositeCommand() {

    // Find a name
    int compSuffix = 0;
    do {
        mName = (compSuffix==0)?QString("comp"):(QObject::tr("comp_") + QString::number(compSuffix));
        compSuffix++;
    }
    while (PM()->findCompositeByName(mName)!=nullptr);
    mRef = PM()->createAssetRef();
    ok = true;
}

void NewCompositeCommand::undo()
{
    Composite* c = PM()->composites.take(mRef);
    delete c;
    MainWindow::Instance()->partListChanged();
}

void NewCompositeCommand::redo()
{
    Composite* comp = new Composite;
    comp->root = -1;
    comp->name = mName;
    comp->ref = mRef;
    PM()->composites.insert(comp->ref, comp);
    MainWindow::Instance()->partListChanged();
    MainWindow::Instance()->openCompositeWidget(mRef);
}

CopyCompositeCommand::CopyCompositeCommand(AssetRef ref){
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

void CopyCompositeCommand::undo(){
    Composite* copy = PM()->composites.take(mCopy);
    delete copy;
    MainWindow::Instance()->partListChanged();
}

void CopyCompositeCommand::redo(){
    const Composite* comp = PM()->getComposite(mOriginal);
    Composite* copy = new Composite;    
    mCopy = PM()->createAssetRef();
    copy->ref = mCopy;
    copy->name = mNewCompositeName;
    copy->root = comp->root;
    copy->properties = comp->properties;
    copy->children = comp->children;
    copy->childrenMap = comp->childrenMap;
    PM()->composites.insert(copy->ref, copy);

    MainWindow::Instance()->partListChanged();
}

DeleteCompositeCommand::DeleteCompositeCommand(AssetRef ref): mRef(ref), mCopy(nullptr){
    ok = PM()->hasComposite(ref);
}

DeleteCompositeCommand::~DeleteCompositeCommand(){
    delete mCopy;
}

void DeleteCompositeCommand::undo()
{
    PM()->composites.insert(mRef, mCopy);
    mCopy = nullptr;
    MainWindow::Instance()->partListChanged();
}

void DeleteCompositeCommand::redo()
{
    mCopy = PM()->composites.take(mRef);
    MainWindow::Instance()->partListChanged();
}

RenameCompositeCommand::RenameCompositeCommand(AssetRef ref, QString newName):mRef(ref),mNewName(newName){
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

void RenameCompositeCommand::undo(){
    Composite* p = PM()->getComposite(mRef);
    p->name = mOldName;

    MainWindow::Instance()->compositeRenamed(mRef, mOldName);
}

void RenameCompositeCommand::redo(){
    Composite* p = PM()->getComposite(mRef);
    p->name = mNewName;

    MainWindow::Instance()->compositeRenamed(mRef, mNewName);
}






















NewModeCommand::NewModeCommand(AssetRef part, const QString& copyModeName):mPart(part), mCopyModeName(copyModeName){
    ok = PM()->hasPart(mPart) && PM()->getPart(mPart)->modes.contains(mCopyModeName);

    if (ok){
        // Find a name
        int suffix = 0;
        Part* p = PM()->parts.value(mPart);
        QStringList modeList = p->modes.keys();
        do {
            mModeName = QString("m") + QString("%1").arg(suffix, 3, 10, QChar('0'));
            suffix++;

        } while (modeList.contains(mModeName));
    }
}

void NewModeCommand::undo(){
    // remove the mode..
    Part* p = PM()->getPart(mPart);
    Part::Mode mode = p->modes.take(mModeName);
    foreach(QImage* img, mode.images){
        if (img) delete img;
    }

    MainWindow::Instance()->partModesChanged(mPart);
}

void NewModeCommand::redo(){
    Part* p = PM()->parts.value(mPart);
    Part::Mode copyMode = p->modes.value(mCopyModeName);
    Part::Mode m;
    m.numFrames = 1;
    m.width = copyMode.width;
    m.height = copyMode.height;
    m.numPivots = copyMode.numPivots;
    m.framesPerSecond = copyMode.framesPerSecond;
    for(int k=0;k<m.numFrames;k++){
        for(int p=0;p<MAX_PIVOTS;p++){
            m.pivots[p].push_back(QPoint(0,0));
        }
        m.anchor.push_back(QPoint(0,0));
        QImage* img = new QImage(m.width, m.height, QImage::Format_ARGB32);
        m.images.push_back(img);
        img->fill(QColor(255,255,255,0));
    }
    p->modes.insert(mModeName,m);

    MainWindow::Instance()->partModesChanged(mPart);
}


DeleteModeCommand::DeleteModeCommand(AssetRef part, const QString& modeName):mPart(part),mModeName(modeName){
    // mModeCopy
    ok = PM()->hasPart(mPart) && PM()->getPart(mPart)->modes.contains(mModeName);
}

void DeleteModeCommand::undo(){
    // re-add the mode..
    Part* p = PM()->getPart(mPart);
    p->modes.insert(mModeName, mModeCopy);
    MainWindow::Instance()->partModesChanged(mPart);
}

void DeleteModeCommand::redo(){
    // remove the mode..
    Part* p = PM()->getPart(mPart);
    mModeCopy = p->modes.take(mModeName);
    MainWindow::Instance()->partModesChanged(mPart);
}


ResetModeCommand::ResetModeCommand(AssetRef part, const QString& modeName):mPart(part),mModeName(modeName){
    // mModeCopy
    ok = PM()->hasPart(mPart) && PM()->getPart(mPart)->modes.contains(mModeName);
}

void ResetModeCommand::undo(){
    // re-add the mode..
    Part* p = PM()->getPart(mPart);
    Part::Mode& mode = p->modes[mModeName];
    for(int k=0;k<mode.numFrames;k++){
        delete mode.images.at(k);
    }
    p->modes[mModeName] = mModeCopy;

    MainWindow::Instance()->partModesChanged(mPart);
}

void ResetModeCommand::redo(){
    // remove the mode..
    Part* p = PM()->getPart(mPart);
    Part::Mode& mode = p->modes[mModeName];
    mModeCopy = mode;
    mModeCopy.anchor = mode.anchor;
    mModeCopy.images = mode.images;
    for(int p=0;p<MAX_PIVOTS;p++){
        mModeCopy.pivots[p] = mode.pivots[p];
    }

    mode.images.clear();
    mode.anchor.clear();
    for(int p=0;p<MAX_PIVOTS;p++){
        mode.pivots[p].clear();
    }

    QImage* newImage = new QImage(mode.width, mode.height, QImage::Format_ARGB32);
    newImage->fill(0x00FFFFFF);
    mode.images.push_back(newImage);
    mode.anchor.push_back(QPoint(0,0));
    for(int p=0;p<MAX_PIVOTS;p++){
        mode.pivots[p].push_back(QPoint(0,0));
    }
    mode.numPivots = 0;
    mode.numFrames = 1;
    mode.framesPerSecond = 24;

    MainWindow::Instance()->partModesChanged(mPart);
}


CopyModeCommand::CopyModeCommand(AssetRef part, const QString& modeName):mPart(part), mModeName(modeName){
    ok = PM()->hasPart(mPart) && PM()->getPart(mPart)->modes.contains(mModeName);

    if (ok){
        // Find a name
        int suffix = 0;
        Part* p = PM()->parts.value(mPart);
        QStringList modeList = p->modes.keys();
        do {
            mNewModeName = QString("m") + QString("%1").arg(suffix, 3, 10, QChar('0'));
            suffix++;

        } while (modeList.contains(mNewModeName));
    }
}

void CopyModeCommand::undo(){
    // remove the mode..
    Part* p = PM()->getPart(mPart);
    Part::Mode mode = p->modes.take(mNewModeName);
    foreach(QImage* img, mode.images){
        if (img) delete img;
    }
    MainWindow::Instance()->partModesChanged(mPart);
}

void CopyModeCommand::redo(){
    Part* p = PM()->getPart(mPart);
    Part::Mode copyMode = p->modes.value(mModeName);
    Part::Mode m;
    m.numFrames = copyMode.numFrames;
    m.width = copyMode.width;
    m.height = copyMode.height;
    m.numPivots = copyMode.numPivots;
    m.framesPerSecond = copyMode.framesPerSecond;
    for(int p=0;p<MAX_PIVOTS;p++)
        m.pivots[p] = copyMode.pivots[p];
    m.anchor = copyMode.anchor;
    for(int k=0;k<m.numFrames;k++){
        QImage* img = new QImage(copyMode.images.at(k)->copy());
        m.images.push_back(img);
    }
    p->modes.insert(mNewModeName,m);
    MainWindow::Instance()->partModesChanged(mPart);
}


RenameModeCommand::RenameModeCommand(AssetRef part, const QString& oldModeName, const QString& newModeName)
    :mPart(part), mOldModeName(oldModeName), mNewModeName(newModeName){
    mNewModeName = mNewModeName.trimmed(); // .simplified();
    mNewModeName.replace(' ','_');

    if (mNewModeName.size()==0){
        mNewModeName = "_";
    }

    ok =PM()->hasPart(mPart) && PM()->getPart(mPart)->modes.contains(mOldModeName) && !PM()->getPart(mPart)->modes.contains(mNewModeName);
}

void RenameModeCommand::undo(){
    Part* p = PM()->getPart(mPart);
    Part::Mode m = p->modes.take(mNewModeName);
    p->modes.insert(mOldModeName, m);

    MainWindow::Instance()->partModeRenamed(mPart, mNewModeName, mOldModeName);
}

void RenameModeCommand::redo(){
     Part* p = PM()->getPart(mPart);
     Part::Mode m = p->modes.take(mOldModeName);
     p->modes.insert(mNewModeName, m);

     MainWindow::Instance()->partModeRenamed(mPart, mOldModeName, mNewModeName);
}






DrawOnPartCommand::DrawOnPartCommand(AssetRef part, QString mode, int frame, QImage data, QPoint offset)
    :mPart(part),mMode(mode),mFrame(frame),mData(data),mOffset(offset){
    Part* p = PM()->getPart(mPart);
    ok = p &&
            p->modes.contains(mode) &&
            p->modes[mode].numFrames>=frame &&
            p->modes[mode].images.at(frame)!=nullptr;
}

void DrawOnPartCommand::undo(){
    //qDebug() << "DrawOnPartCommand::undo()";
    // Reload the old frame
    QImage* img = PM()->getPart(mPart)->modes[mMode].images.at(mFrame);
    QPainter painter(img);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(0, 0, mOldFrame);
    // *img = mOldFrame;

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPart, mMode, mFrame);
}

void DrawOnPartCommand::redo(){    
    //qDebug() << "DrawOnPartCommand::redo()";
    // Record the old frame
    // Draw the image into the part
    QImage* img = PM()->getPart(mPart)->modes[mMode].images.at(mFrame);
    mOldFrame = img->copy();
    QPainter painter(img);
    painter.drawImage(mOffset.x(), mOffset.y(), mData);

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPart, mMode, mFrame);
}

EraseOnPartCommand::EraseOnPartCommand(AssetRef part, QString mode, int frame, QImage data, QPoint offset)
    :mPart(part),mMode(mode),mFrame(frame),mData(data),mOffset(offset){
     Part* p = PM()->getPart(part);
    ok = p &&
            p->modes.contains(mode) &&
            p->modes[mode].numFrames>=frame &&
            p->modes[mode].images.at(frame)!=nullptr;
}

void EraseOnPartCommand::undo(){
    // Reload the old frame
    QImage* img = PM()->getPart(mPart)->modes[mMode].images.at(mFrame);
    QPainter painter(img);
    painter.drawImage(0, 0, mOldFrame);
    // *img = mOldFrame;

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPart, mMode, mFrame);
}

void EraseOnPartCommand::redo(){
    // Record the old frame
    // Draw the image into the part
    QImage* img = PM()->getPart(mPart)->modes[mMode].images.at(mFrame);
    mOldFrame = *img;
    QPainter painter(img);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    painter.drawImage(mOffset.x(), mOffset.y(), mData);

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPart, mMode, mFrame);
}



NewFrameCommand::NewFrameCommand(AssetRef part, QString modeName, int index)
    :mPart(part), mModeName(modeName), mIndex(index){
    ok = PM()->hasPart(part) &&
         PM()->getPart(part)->modes.contains(modeName);
    if (ok){
        int numFrames = PM()->getPart(part)->modes[modeName].numFrames;
        ok = ok && (index>=0 && index<=numFrames);
    }
}

void NewFrameCommand::undo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    QImage* img = mode.images.takeAt(mIndex);
    if (img) delete img;
    mode.anchor.removeAt(mIndex);
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].removeAt(mIndex);
    }
    mode.numFrames--;

    MainWindow::Instance()->partFramesUpdated(mPart, mModeName);
}

void NewFrameCommand::redo(){
    // Create the new frame
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    QImage* image = new QImage(mode.width, mode.height, QImage::Format_ARGB32);
    image->fill(0x00FFFFFF);
    mode.images.insert(mIndex, image);

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

CopyFrameCommand::CopyFrameCommand(AssetRef part, QString modeName, int index)
    :mPart(part), mModeName(modeName), mIndex(index){
    ok = PM()->hasPart(part) &&
         PM()->getPart(part)->modes.contains(modeName);
    if (ok){
        int numFrames = PM()->getPart(mPart)->modes[modeName].numFrames;
        ok = ok && (index>=0 && index<numFrames);
    }
}

void CopyFrameCommand::undo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    QImage* img = mode.images.takeAt(mIndex+1);
    if (img) delete img;
    mode.anchor.removeAt(mIndex+1);
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].removeAt(mIndex+1);
    }
    mode.numFrames--;
    MainWindow::Instance()->partFramesUpdated(mPart, mModeName);
}

void CopyFrameCommand::redo(){
    // Create the new frame
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];

    QImage* image = nullptr;
    if (mIndex<mode.numFrames){
        mode.anchor.insert(mIndex+1, mode.anchor.at(mIndex));
        image = new QImage(mode.images.at(mIndex)->copy());
    }
    else if (mode.numFrames>0){
        mode.anchor.insert(mIndex+1, mode.anchor.at(0));
        image = new QImage(mode.images.at(0)->copy());
    }
    else {
       mode.anchor.insert(mIndex+1, QPoint(0,0));
       image = new QImage(mode.width, mode.height, QImage::Format_ARGB32);
       image->fill(0x00FFFFFF);
    }
    mode.images.insert(mIndex+1, image);

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

DeleteFrameCommand::DeleteFrameCommand(AssetRef part, QString modeName, int index)
    :mPart(part), mModeName(modeName), mIndex(index){
    ok = PM()->hasPart(part) &&
         PM()->getPart(part)->modes.contains(modeName);
    if (ok){
        int numFrames = PM()->getPart(mPart)->modes[modeName].numFrames;
        ok = ok && (index>=0 && index<numFrames);
    }
}

void DeleteFrameCommand::undo(){
    // Create the new frame
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    QImage* image = new QImage(mImage);
    mode.images.insert(mIndex, image);
    mode.anchor.insert(mIndex, mAnchor);
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].insert(mIndex, mPivots[i]);
    }
    mode.numFrames++;

    MainWindow::Instance()->partFramesUpdated(mPart, mModeName);

}

void DeleteFrameCommand::redo(){
    // NB: Remember old frame info (image, etc..)
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    QImage* img = mode.images.takeAt(mIndex);
    mImage = img->copy();
    if (img) delete img;
    mAnchor = mode.anchor.takeAt(mIndex);
    for(int i=0;i<MAX_PIVOTS;i++){
        mPivots[i] = mode.pivots[i].takeAt(mIndex);
    }
    mode.numFrames--;

    MainWindow::Instance()->partFramesUpdated(mPart, mModeName);
}





UpdateAnchorAndPivotsCommand::UpdateAnchorAndPivotsCommand(AssetRef part, QString modeName, int index, QPoint anchor, QPoint p1, QPoint p2, QPoint p3, QPoint p4)
    :mPart(part), mModeName(modeName), mIndex(index), mAnchor(anchor) {
    mPivots[0] = p1;
    mPivots[1] = p2;
    mPivots[2] = p3;
    mPivots[3] = p4;

    Part* p = PM()->getPart(mPart);
    ok = p &&
            p->modes.contains(mModeName) &&
            p->modes[mModeName].numFrames>mIndex &&
            p->modes[mModeName].images.at(mIndex)!=nullptr;
}

void UpdateAnchorAndPivotsCommand::undo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mode.anchor.replace(mIndex, mOldAnchor);
    for(int i=0;i<mode.numPivots;i++){
        mode.pivots[i].replace(mIndex, mOldPivots[i]);
    }
    MainWindow::Instance()->partFrameUpdated(mPart, mModeName, mIndex);
}

void UpdateAnchorAndPivotsCommand::redo(){
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

ChangeNumPivotsCommand::ChangeNumPivotsCommand(AssetRef part, QString modeName, int numPivots)
    :mPart(part), mModeName(modeName), mNumPivots(numPivots){
    ok = PM()->hasPart(part) &&
         PM()->getPart(part)->modes.contains(modeName);
}

void ChangeNumPivotsCommand::undo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mode.numPivots = mOldNumPivots;

    MainWindow::Instance()->partNumPivotsUpdated(mPart, mModeName);
}

void ChangeNumPivotsCommand::redo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mOldNumPivots = mode.numPivots;
    mode.numPivots = mNumPivots;

    MainWindow::Instance()->partNumPivotsUpdated(mPart, mModeName);
}

ChangeModeSizeCommand::ChangeModeSizeCommand(AssetRef part, QString modeName, int width, int height, int offsetx, int offsety)
   :mPart(part), mModeName(modeName), mWidth(width), mHeight(height), mOffsetX(offsetx), mOffsetY(offsety){
    ok = PM()->hasPart(mPart) &&
         PM()->getPart(mPart)->modes.contains(modeName);
    ok = ok && mWidth>0 && mHeight>0;
}

void ChangeModeSizeCommand::undo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];

    for(int k=0;k<mode.numFrames;k++){
        delete mode.images.at(k);
    }
    mode.images.clear();

    part->modes[mModeName] = mOldMode;

     MainWindow::Instance()->partModesChanged(mPart);
}

void ChangeModeSizeCommand::redo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mOldWidth = mode.width;
    mOldHeight = mode.height;

    // make copy of old mode
    mOldMode = mode;
    mOldMode.images.clear();
    for(int p=0;p<MAX_PIVOTS;p++)
        mOldMode.pivots[p] = mOldMode.pivots[p];
    for(int k=0;k<mOldMode.numFrames;k++){
        // QImage* img = new QImage(mode.images.at(k)->copy());
        mOldMode.images.push_back(mode.images.at(k)); // this ref will be removed below anyway
    }

    // Modify mode to new dimensions
    mode.width = mWidth;
    mode.height = mHeight;
    for(int k=0;k<mode.numFrames;k++){
        QImage* newImage = new QImage(mWidth, mHeight, QImage::Format_ARGB32);
        newImage->fill(0x00FFFFFF);
        QPainter painter(newImage);
        painter.drawImage(mOffsetX,mOffsetY,*mode.images.at(k));
        painter.end();
        mode.images.replace(k, newImage);

        mode.anchor[k] += QPoint(mOffsetX,mOffsetY);
        for(int p=0;p<mode.numPivots;p++){
            mode.pivots[p][k] += QPoint(mOffsetX,mOffsetY);
        }
    }

    // mOldWidth, mOldHeight
    // save mOldMode

    MainWindow::Instance()->partModesChanged(mPart);
}

ChangeModeFPSCommand::ChangeModeFPSCommand(AssetRef part, QString modeName, int fps)
    :mPart(part), mModeName(modeName), mFPS(fps){
    ok = PM()->hasPart(mPart) &&
         PM()->getPart(mPart)->modes.contains(modeName);
    ok = ok && mFPS>0;
}

void ChangeModeFPSCommand::undo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mode.framesPerSecond = mOldFPS;

    MainWindow::Instance()->partModesChanged(mPart);
}

void ChangeModeFPSCommand::redo(){
    Part* part = PM()->getPart(mPart);
    Part::Mode& mode = part->modes[mModeName];
    mOldFPS = mode.framesPerSecond;
    mode.framesPerSecond = mFPS;

    MainWindow::Instance()->partModesChanged(mPart);
}








EditCompositeChildCommand::EditCompositeChildCommand(AssetRef comp, const QString& childName, AssetRef newPart, int newZ, int newParent, int newParentPivot)
    :mComp(comp), mChildName(childName), mNewPart(newPart), mNewParent(newParent), mNewParentPivot(newParentPivot), mNewZ(newZ)
{
    ok = PM()->hasComposite(mComp) && PM()->getComposite(mComp)->childrenMap.contains(mChildName);
}

void EditCompositeChildCommand::undo(){
    Composite* comp = PM()->getComposite(mComp);
    Composite::Child& child = comp->childrenMap[mChildName];

    child.parent = mOldParent;
    child.parentPivot = mOldParentPivot;

    child.part = mOldPart;
    child.z = mOldZ;

    // qDebug() << "EditCompositeChildCommand: Updating child: " << mChildName << child.part << mNewZ << child.parent << child.parentPivot;

    // Fix all parent/children links...
    if (mOldParent!=mNewParent){
        // remove child from new parent
        if (mNewParent>=0 && mNewParent<comp->children.count()){
            Composite::Child& parent = comp->childrenMap[comp->children[mNewParent]];
            int index = parent.children.indexOf(child.index);
            if (index>=0){
                // qDebug() << "EditCompositeChildCommand: Removing child from " << comp->children[mOldParent];
                parent.children.removeAt(index);
            }
        }

        // add child back to new parent
        if (mOldParent!=child.index && mOldParent>=0 && mOldParent<comp->children.count()){
            Composite::Child& parent = comp->childrenMap[comp->children[mOldParent]];
            int index = parent.children.indexOf(child.index);
            if (index==-1){
                // qDebug() << "EditCompositeChildCommand: Adding child to " << comp->children[mNewParent];
                parent.children.append(child.index);
            }
        }
    }

    // Restore root
    comp->root = mOldRoot;

    // Update comp..
    MainWindow::Instance()->compositeUpdatedMinorChanges(mComp);
}

void EditCompositeChildCommand::redo(){
    Composite* comp = PM()->getComposite(mComp);
    Composite::Child& child = comp->childrenMap[mChildName];

    mOldPart = child.part;
    Part* part = PM()->getPart(child.part);
    // child.part = part?part->ref:AssetRef();
    // mOldPartName = part?part->name:"";

    mOldZ = child.z;
    mOldParent = child.parent;
    mOldParentPivot = child.parentPivot;

    child.parent = std::max(-1, mNewParent);
    child.parentPivot = std::max(-1, mNewParentPivot);

    // Part* newPart = PM()->getPart(mNewPart);
    child.part = mNewPart; // newPart?newPart->ref:AssetRef();
    child.z = mNewZ;

    // qDebug() << "EditCompositeChildCommand: Updating child: " << mChildName << child.part << mNewZ << child.parent << child.parentPivot;

    // Fix all parent/children links...
    if (mOldParent!=mNewParent){
        // remove child from old parent
        if (mOldParent>=0 && mOldParent<comp->children.count()){
            Composite::Child& parent = comp->childrenMap[comp->children[mOldParent]];
            int index = parent.children.indexOf(child.index);
            if (index>=0){
                // qDebug() << "EditCompositeChildCommand: Removing child from " << comp->children[mOldParent];
                parent.children.removeAt(index);
            }
        }

        // add child to new parent
        // NB: cycles and bad relationships are just ignored in the comp widget (but allowed in the spec)
        if (mNewParent!=child.index && mNewParent>=0 && mNewParent<comp->children.count()){
            Composite::Child& parent = comp->childrenMap[comp->children[mNewParent]];
            int index = parent.children.indexOf(child.index);
            if (index==-1){
                // qDebug() << "EditCompositeChildCommand: Adding child to " << comp->children[mNewParent];
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
            // qDebug() << "EditCompositeChildCommand: Setting root to " << comp->root;
            break;
        }
    }

    // Update comp..
    MainWindow::Instance()->compositeUpdatedMinorChanges(mComp);
}

NewCompositeChildCommand::NewCompositeChildCommand(AssetRef comp):
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

void NewCompositeChildCommand::undo(){
    // Delete the child..
    Composite* comp = PM()->getComposite(mComp);
    comp->children.removeAll(mChildName);
    comp->childrenMap.remove(mChildName);

    // Update widgets
    MainWindow::Instance()->compositeUpdated(mComp);
}

void NewCompositeChildCommand::redo(){
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

EditCompositeChildNameCommand::EditCompositeChildNameCommand(AssetRef ref, const QString& child, const QString& newChildName)
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

void EditCompositeChildNameCommand::undo(){
    // Unchange the child name
    Composite* comp = PM()->getComposite(mComp);
    comp->children.replace(comp->children.indexOf(mNewChildName), mOldChildName);
    comp->childrenMap.insert(mOldChildName, comp->childrenMap.take(mNewChildName));
    MainWindow::Instance()->compositeUpdated(mComp);
}

void EditCompositeChildNameCommand::redo(){
    // Change the child name
    Composite* comp = PM()->getComposite(mComp);
    comp->children.replace(comp->children.indexOf(mOldChildName), mNewChildName);
    comp->childrenMap.insert(mNewChildName, comp->childrenMap.take(mOldChildName));
    MainWindow::Instance()->compositeUpdated(mComp);
}

DeleteCompositeChildCommand::DeleteCompositeChildCommand(AssetRef ref, const QString& childName)
    :mComp(ref), mChildName(childName){

    Composite* comp = PM()->getComposite(mComp);
    ok = comp && comp->children.contains(childName) && comp->childrenMap.contains(childName);
    if (ok){
        mChildIndex = comp->children.indexOf(childName);
        ok = (mChildIndex>=0);
    }
}

void DeleteCompositeChildCommand::undo(){
    Composite* comp = new Composite(mCompCopy);
    delete PM()->composites.take(mComp);
    PM()->composites.insert(mComp, comp);
    MainWindow::Instance()->compositeUpdated(mComp);
}

int FixIndex(int i, int ci){
    if (i==ci) return -1;
    else if (i>ci) return i-1;
    else return i;
}

void DeleteCompositeChildCommand::redo(){
    Composite* comp = PM()->getComposite(mComp);
    mCompCopy = *comp; // make a copy so we can undo it

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


ChangePropertiesCommand::ChangePropertiesCommand(AssetRef part, QString properties)
    :mPart(part), mProperties(properties){
    ok = PM()->hasPart(mPart);
}

void ChangePropertiesCommand::undo(){
    Part* part = PM()->getPart(mPart);
    part->properties = mOldProperties;
    MainWindow::Instance()->partPropertiesUpdated(mPart);
}

void ChangePropertiesCommand::redo(){
    Part* part = PM()->getPart(mPart);
    mOldProperties = part->properties;
    part->properties = mProperties;

    MainWindow::Instance()->partPropertiesUpdated(mPart);
}

ChangeCompPropertiesCommand::ChangeCompPropertiesCommand(AssetRef comp, QString properties)
    :mComp(comp), mProperties(properties){
    ok = PM()->hasComposite(mComp);
}

void ChangeCompPropertiesCommand::undo(){
    Composite* comp = PM()->getComposite(mComp);
    comp->properties = mOldProperties;

    MainWindow::Instance()->compPropertiesUpdated(mComp);
}

void ChangeCompPropertiesCommand::redo(){
    Composite* comp = PM()->getComposite(mComp);
    mOldProperties = comp->properties;
    comp->properties = mProperties;

    MainWindow::Instance()->compPropertiesUpdated(mComp);
}

