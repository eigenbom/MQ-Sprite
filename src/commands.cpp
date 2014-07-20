#include "commands.h"
#include "mainwindow.h"
#include <QObject>
#include <QString>

static int sNewCompositeSuffix = 0;
static int sNewModeSuffix = 0;

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
    } while (PM()->hasPart(name));

    Part* part = new Part;
    part->ref = mUuid;
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
    part->modes.insert("dflt", mode);
    PM()->parts.insert(part->ref, part);
    MainWindow::Instance()->partListChanged();


    // TODO: openPartWidget(AssetRef)
    MainWindow::Instance()->openPartWidget(name);

}

CopyPartCommand::CopyPartCommand(AssetRef ref){
    mOriginal = ref;
    ok = PM()->hasPart(ref);
    if (ok){
        Part* part = PM()->getPart(ref);
        int copyNumber = 0;
        do {
            mNewPartName = part->name + "_" + QString::number(copyNumber++);
        } while (PM()->hasPart(mNewPartName));
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
    } while ((PM()->hasPart(mNewName)));
}

void RenamePartCommand::undo(){
    // Part* p = PM()->parts[mNewName];
    // PM()->parts.remove(mNewName);
    // PM()->parts.insert(mOldName, p);

    Part* p = PM()->parts[mRef];
    p->name = mOldName;

    // update comps too...
    foreach(Composite* comp, PM()->composites.values()){        
        QMutableMapIterator<QString,Composite::Child> it(comp->childrenMap);
        while (it.hasNext()){
            it.next();
            Composite::Child& ch = it.value();
            if (ch.part==mNewName){
                ch.part = mOldName;
            }
        }
    }

    MainWindow::Instance()->partRenamed(mNewName, mOldName);
}

void RenamePartCommand::redo(){

    // Part* p = PM()->parts[mOldName];
    // PM()->parts.remove(mOldName);
    // PM()->parts.insert(mNewName, p);
    Part* p = PM()->parts[mRef];
    mOldName = p->name;
    p->name = mNewName;

    // update comps too...
    foreach(Composite* comp, PM()->composites.values()){
        QMutableMapIterator<QString,Composite::Child> it(comp->childrenMap);
        while (it.hasNext()){
            it.next();
            Composite::Child& ch = it.value();
            if (ch.part==mOldName){
                ch.part = mNewName;
            }
        }
    }

    MainWindow::Instance()->partRenamed(mOldName, mNewName);
}

NewCompositeCommand::NewCompositeCommand() {

    // Find a name
    int compSuffix = 0;
    do {
        mName = (compSuffix==0)?QString("comp"):(QObject::tr("comp_") + QString::number(compSuffix));
        compSuffix++;
    }
    while (PM()->hasComposite(mName));
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
    MainWindow::Instance()->openCompositeWidget(mName);
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
        while (PM()->hasComposite(mNewCompositeName));
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
        while (PM()->hasComposite(mNewName));
    }
}

void RenameCompositeCommand::undo(){
    Composite* p = PM()->getComposite(mRef);
    p->name = mOldName;

    MainWindow::Instance()->compositeRenamed(mNewName, mOldName);
}

void RenameCompositeCommand::redo(){
    Composite* p = PM()->getComposite(mRef);
    p->name = mNewName;

    MainWindow::Instance()->compositeRenamed(mOldName, mNewName);
}






















/*

NewModeCommand::NewModeCommand(const QString& partName, const QString& copyModeName):mPartName(partName), mCopyModeName(copyModeName){
    ok = PM()->parts.contains(mPartName) && PM()->parts.value(mPartName)->modes.contains(mCopyModeName);

    if (ok){
        // Find a name
        QString number = QString("%1").arg(sNewModeSuffix++, 3, 10, QChar('0'));
        mModeName = QString("m") + number;
        if (sNewModeSuffix>999) sNewModeSuffix = 0;
        Part* p = PM()->parts.value(mPartName);
        QStringList list = p->modes.keys();
        while (list.contains(mModeName)){
            QString number = QString("%1").arg(sNewModeSuffix++, 3, 10, QChar('0'));
            mModeName = QString("m") + number;
            if (sNewModeSuffix>999) sNewModeSuffix = 0;
        }
    }
}

void NewModeCommand::undo(){
    // remove the mode..
    Part* p = PM()->parts.value(mPartName);
    Part::Mode mode = p->modes.take(mModeName);
    foreach(QImage* img, mode.images){
        if (img) delete img;
    }

    MainWindow::Instance()->partModesChanged(mPartName);
}

void NewModeCommand::redo(){
    Part* p = PM()->parts.value(mPartName);
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

    MainWindow::Instance()->partModesChanged(mPartName);
}

DeleteModeCommand::DeleteModeCommand(const QString& partName, const QString& modeName):mPartName(partName),mModeName(modeName){
    // mModeCopy
    ok = PM()->parts.contains(mPartName) && PM()->parts.value(mPartName)->modes.contains(mModeName);
}

void DeleteModeCommand::undo(){
    // re-add the mode..
    Part* p = PM()->parts.value(mPartName);
    p->modes.insert(mModeName, mModeCopy);
    MainWindow::Instance()->partModesChanged(mPartName);
}

void DeleteModeCommand::redo(){
    // remove the mode..
    Part* p = PM()->parts.value(mPartName);
    mModeCopy = p->modes.take(mModeName);
    MainWindow::Instance()->partModesChanged(mPartName);
}

ResetModeCommand::ResetModeCommand(const QString& partName, const QString& modeName):mPartName(partName),mModeName(modeName){
    // mModeCopy
    ok = PM()->parts.contains(mPartName) && PM()->parts.value(mPartName)->modes.contains(mModeName);
}

void ResetModeCommand::undo(){
    // re-add the mode..
    Part* p = PM()->parts.value(mPartName);
    Part::Mode& mode = p->modes[mModeName];
    for(int k=0;k<mode.numFrames;k++){
        delete mode.images.at(k);
    }
    p->modes[mModeName] = mModeCopy;

    MainWindow::Instance()->partModesChanged(mPartName);
}

void ResetModeCommand::redo(){
    // remove the mode..
    Part* p = PM()->parts.value(mPartName);
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

    MainWindow::Instance()->partModesChanged(mPartName);
}

CopyModeCommand::CopyModeCommand(const QString& partName, const QString& modeName):mPartName(partName), mModeName(modeName){
    ok = PM()->parts.contains(mPartName) && PM()->parts.value(mPartName)->modes.contains(mModeName);

    if (ok){
        // Find a name
        QString number = QString("%1").arg(sNewModeSuffix++, 3, 10, QChar('0'));
        mNewModeName = QString("m") + number;
        if (sNewModeSuffix>999) sNewModeSuffix = 0;
        Part* p = PM()->parts.value(mPartName);
        QStringList list = p->modes.keys();
        while (list.contains(mNewModeName)){
            QString number = QString("%1").arg(sNewModeSuffix++, 3, 10, QChar('0'));
            mNewModeName = QString("m") + number;
            if (sNewModeSuffix>999) sNewModeSuffix = 0;
        }
    }
}

void CopyModeCommand::undo(){
    // remove the mode..
    Part* p = PM()->parts.value(mPartName);
    Part::Mode mode = p->modes.take(mNewModeName);
    foreach(QImage* img, mode.images){
        if (img) delete img;
    }
    MainWindow::Instance()->partModesChanged(mPartName);
}

void CopyModeCommand::redo(){
    Part* p = PM()->parts.value(mPartName);
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
    MainWindow::Instance()->partModesChanged(mPartName);
}

RenameModeCommand::RenameModeCommand(const QString& partName, const QString& oldModeName, const QString& newModeName)
    :mPartName(partName), mOldModeName(oldModeName), mNewModeName(newModeName){
    mNewModeName = mNewModeName.trimmed(); // .simplified();
    mNewModeName.replace(' ','_');

    if (mNewModeName.size()==0){
        mNewModeName = "_";
    }

    ok = PM()->parts.contains(mPartName) && PM()->parts.value(mPartName)->modes.contains(mOldModeName) && !PM()->parts.value(mPartName)->modes.contains(mNewModeName);
}

void RenameModeCommand::undo(){
    Part* p = PM()->parts.value(mPartName);
    Part::Mode m = p->modes.take(mNewModeName);
    p->modes.insert(mOldModeName, m);

    MainWindow::Instance()->partModeRenamed(mPartName, mNewModeName, mOldModeName);
}

void RenameModeCommand::redo(){
     Part* p = PM()->parts.value(mPartName);
     Part::Mode m = p->modes.take(mOldModeName);
     p->modes.insert(mNewModeName, m);

     MainWindow::Instance()->partModeRenamed(mPartName, mOldModeName, mNewModeName);
}





DrawOnPartCommand::DrawOnPartCommand(QString partName, QString mode, int frame, QImage data, QPoint offset)
    :mPartName(partName),mMode(mode),mFrame(frame),mData(data),mOffset(offset){
    ok = PM()->parts.contains(partName) &&
            PM()->parts[partName]->modes.contains(mode) &&
            PM()->parts[partName]->modes[mode].numFrames>=frame &&
            PM()->parts[partName]->modes[mode].images.at(frame)!=NULL;
}

void DrawOnPartCommand::undo(){
    //qDebug() << "DrawOnPartCommand::undo()";
    // Reload the old frame
    QImage* img = PM()->parts[mPartName]->modes[mMode].images.at(mFrame);
    QPainter painter(img);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(0, 0, mOldFrame);
    // *img = mOldFrame;

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPartName, mMode, mFrame);
}

void DrawOnPartCommand::redo(){    
    //qDebug() << "DrawOnPartCommand::redo()";
    // Record the old frame
    // Draw the image into the part
    QImage* img = PM()->parts[mPartName]->modes[mMode].images.at(mFrame);
    mOldFrame = img->copy();
    QPainter painter(img);
    painter.drawImage(mOffset.x(), mOffset.y(), mData);

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPartName, mMode, mFrame);
}

EraseOnPartCommand::EraseOnPartCommand(QString partName, QString mode, int frame, QImage data, QPoint offset)
    :mPartName(partName),mMode(mode),mFrame(frame),mData(data),mOffset(offset){
    ok = PM()->parts.contains(partName) &&
            PM()->parts[partName]->modes.contains(mode) &&
            PM()->parts[partName]->modes[mode].numFrames>=frame &&
            PM()->parts[partName]->modes[mode].images.at(frame)!=NULL;
}

void EraseOnPartCommand::undo(){
    // Reload the old frame
    QImage* img = PM()->parts[mPartName]->modes[mMode].images.at(mFrame);
    QPainter painter(img);
    painter.drawImage(0, 0, mOldFrame);
    // *img = mOldFrame;

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPartName, mMode, mFrame);
}

void EraseOnPartCommand::redo(){
    // Record the old frame
    // Draw the image into the part
    QImage* img = PM()->parts[mPartName]->modes[mMode].images.at(mFrame);
    mOldFrame = *img;
    QPainter painter(img);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    painter.drawImage(mOffset.x(), mOffset.y(), mData);

    // tell everyone that the part has been updated
    MainWindow::Instance()->partFrameUpdated(mPartName, mMode, mFrame);
}

NewFrameCommand::NewFrameCommand(QString partName, QString modeName, int index)
    :mPartName(partName), mModeName(modeName), mIndex(index){
    ok = PM()->parts.contains(partName) &&
         PM()->parts[partName]->modes.contains(modeName);
    if (ok){
        int numFrames = PM()->parts[partName]->modes[modeName].numFrames;
        ok = ok && (index>=0 && index<=numFrames);
    }
}

void NewFrameCommand::undo(){
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    QImage* img = mode.images.takeAt(mIndex);
    if (img) delete img;
    mode.anchor.removeAt(mIndex);
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].removeAt(mIndex);
    }
    mode.numFrames--;

    MainWindow::Instance()->partFramesUpdated(mPartName, mModeName);
}

void NewFrameCommand::redo(){
    // Create the new frame
    Part* part = PM()->parts[mPartName];
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

    MainWindow::Instance()->partFramesUpdated(mPartName, mModeName);
}

CopyFrameCommand::CopyFrameCommand(QString partName, QString modeName, int index)
    :mPartName(partName), mModeName(modeName), mIndex(index){
    ok = PM()->parts.contains(partName) &&
         PM()->parts[partName]->modes.contains(modeName);
    if (ok){
        int numFrames = PM()->parts[partName]->modes[modeName].numFrames;
        ok = ok && (index>=0 && index<numFrames);
    }
}

void CopyFrameCommand::undo(){
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    QImage* img = mode.images.takeAt(mIndex+1);
    if (img) delete img;
    mode.anchor.removeAt(mIndex+1);
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].removeAt(mIndex+1);
    }
    mode.numFrames--;
    MainWindow::Instance()->partFramesUpdated(mPartName, mModeName);
}

void CopyFrameCommand::redo(){
    // Create the new frame
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];

    QImage* image = NULL;
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

    MainWindow::Instance()->partFramesUpdated(mPartName, mModeName);
}

DeleteFrameCommand::DeleteFrameCommand(QString partName, QString modeName, int index)
    :mPartName(partName), mModeName(modeName), mIndex(index){
    ok = PM()->parts.contains(partName) &&
         PM()->parts[partName]->modes.contains(modeName);
    if (ok){
        int numFrames = PM()->parts[partName]->modes[modeName].numFrames;
        ok = ok && (index>=0 && index<numFrames);
    }
}

void DeleteFrameCommand::undo(){
    // Create the new frame
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    QImage* image = new QImage(mImage);
    mode.images.insert(mIndex, image);
    mode.anchor.insert(mIndex, mAnchor);
    for(int i=0;i<MAX_PIVOTS;i++){
        mode.pivots[i].insert(mIndex, mPivots[i]);
    }
    mode.numFrames++;

    MainWindow::Instance()->partFramesUpdated(mPartName, mModeName);

}

void DeleteFrameCommand::redo(){
    // NB: Remember old frame info (image, etc..)
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    QImage* img = mode.images.takeAt(mIndex);
    mImage = img->copy();
    if (img) delete img;
    mAnchor = mode.anchor.takeAt(mIndex);
    for(int i=0;i<MAX_PIVOTS;i++){
        mPivots[i] = mode.pivots[i].takeAt(mIndex);
    }
    mode.numFrames--;

    MainWindow::Instance()->partFramesUpdated(mPartName, mModeName);
}

UpdateAnchorAndPivotsCommand::UpdateAnchorAndPivotsCommand(QString partName, QString modeName, int index, QPoint anchor, QPoint p1, QPoint p2, QPoint p3, QPoint p4)
    :mPartName(partName), mModeName(modeName), mIndex(index), mAnchor(anchor) {
    mPivots[0] = p1;
    mPivots[1] = p2;
    mPivots[2] = p3;
    mPivots[3] = p4;

    ok = PM()->parts.contains(partName) &&
            PM()->parts[partName]->modes.contains(mModeName) &&
            PM()->parts[partName]->modes[mModeName].numFrames>mIndex &&
            PM()->parts[partName]->modes[mModeName].images.at(mIndex)!=NULL;
}

void UpdateAnchorAndPivotsCommand::undo(){
    // qDebug() << "UpdateAnchorAndPivotsCommand::undo()";
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    mode.anchor.replace(mIndex, mOldAnchor);
    for(int i=0;i<mode.numPivots;i++){
        mode.pivots[i].replace(mIndex, mOldPivots[i]);
    }

    MainWindow::Instance()->partFrameUpdated(mPartName, mModeName, mIndex);
}

void UpdateAnchorAndPivotsCommand::redo(){
    // qDebug() << "UpdateAnchorAndPivotsCommand::redo()";

    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    mOldAnchor = mode.anchor.at(mIndex);
    mode.anchor.replace(mIndex, mAnchor);
    for(int i=0;i<mode.numPivots;i++){
        mOldPivots[i] = mode.pivots[i].at(mIndex);
        mode.pivots[i].replace(mIndex, mPivots[i]);
    }

    MainWindow::Instance()->partFrameUpdated(mPartName, mModeName, mIndex);
}

ChangeNumPivotsCommand::ChangeNumPivotsCommand(QString partName, QString modeName, int numPivots)
    :mPartName(partName), mModeName(modeName), mNumPivots(numPivots){
    ok = PM()->parts.contains(partName) &&
         PM()->parts[partName]->modes.contains(modeName);
}

void ChangeNumPivotsCommand::undo(){
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    mode.numPivots = mOldNumPivots;

    MainWindow::Instance()->partNumPivotsUpdated(mPartName, mModeName);
}

void ChangeNumPivotsCommand::redo(){
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    mOldNumPivots = mode.numPivots;
    mode.numPivots = mNumPivots;

    MainWindow::Instance()->partNumPivotsUpdated(mPartName, mModeName);
}

ChangeModeSizeCommand::ChangeModeSizeCommand(QString partName, QString modeName, int width, int height, int offsetx, int offsety):
    mPartName(partName), mModeName(modeName), mWidth(width), mHeight(height), mOffsetX(offsetx), mOffsetY(offsety){
    ok = PM()->parts.contains(partName) &&
         PM()->parts[partName]->modes.contains(modeName);
    ok = ok && mWidth>0 && mHeight>0;
}

void ChangeModeSizeCommand::undo(){
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];

    for(int k=0;k<mode.numFrames;k++){
        delete mode.images.at(k);
    }
    mode.images.clear();

    part->modes[mModeName] = mOldMode;

     MainWindow::Instance()->partModesChanged(mPartName);
}

void ChangeModeSizeCommand::redo(){
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    mOldWidth = mode.width;
    mOldHeight = mode.height;

    qDebug() << "Making copy of old mode";

    // make copy of old mode
    mOldMode = mode;
    mOldMode.images.clear();
    for(int p=0;p<MAX_PIVOTS;p++)
        mOldMode.pivots[p] = mOldMode.pivots[p];
    for(int k=0;k<mOldMode.numFrames;k++){
        // QImage* img = new QImage(mode.images.at(k)->copy());
        mOldMode.images.push_back(mode.images.at(k)); // this ref will be removed below anyway
    }

    qDebug() << "Modify mode to new dimensions";
    // Modify mode to new dimensions
    mode.width = mWidth;
    mode.height = mHeight;
    for(int k=0;k<mode.numFrames;k++){
        qDebug() << "Frame " << k;

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

    qDebug() << "Done";

    // mOldWidth, mOldHeight
    // save mOldMode

    MainWindow::Instance()->partModesChanged(mPartName);
}

ChangeModeFPSCommand::ChangeModeFPSCommand(QString partName, QString modeName, int fps)
    :mPartName(partName), mModeName(modeName), mFPS(fps){
    ok = PM()->parts.contains(partName) &&
         PM()->parts[partName]->modes.contains(modeName);
    ok = ok && mFPS>0;
}

void ChangeModeFPSCommand::undo(){
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    mode.framesPerSecond = mOldFPS;

    MainWindow::Instance()->partModesChanged(mPartName);
}

void ChangeModeFPSCommand::redo(){
    Part* part = PM()->parts[mPartName];
    Part::Mode& mode = part->modes[mModeName];
    mOldFPS = mode.framesPerSecond;
    mode.framesPerSecond = mFPS;

    MainWindow::Instance()->partModesChanged(mPartName);
}

EditCompositeChildCommand::EditCompositeChildCommand(const QString& compName, const QString& childName, const QString& newPartName, int newZ, int newParent, int newParentPivot)
    :mCompName(compName), mChildName(childName), mNewPartName(newPartName), mNewParent(newParent), mNewParentPivot(newParentPivot), mNewZ(newZ)
{
    if (PM()->composites.contains(mCompName)){
        Composite* comp = PM()->composites.value(mCompName);
        if (comp->children.contains(mChildName) && comp->childrenMap.contains(mChildName)){
            ok = true;
        }
        else {
            ok = false;
        }
    }
    else {
        ok = false;
    }

}

void EditCompositeChildCommand::undo(){
    Composite* comp = PM()->composites.value(mCompName);
    Composite::Child& child = comp->childrenMap[mChildName];

    child.parent = mOldParent;
    child.parentPivot = mOldParentPivot;
    child.part = mOldPartName;
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
    MainWindow::Instance()->compositeUpdatedMinorChanges(mCompName);
}

void EditCompositeChildCommand::redo(){
    Composite* comp = PM()->composites.value(mCompName);
    Composite::Child& child = comp->childrenMap[mChildName];
    mOldPartName = child.part;
    mOldZ = child.z;
    mOldParent = child.parent;
    mOldParentPivot = child.parentPivot;

    child.parent = std::max(-1, mNewParent);
    child.parentPivot = std::max(-1, mNewParentPivot);
    child.part = mNewPartName;
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
    MainWindow::Instance()->compositeUpdatedMinorChanges(mCompName);
}

NewCompositeChildCommand::NewCompositeChildCommand(const QString& compName):
    mCompName(compName){
    ok = PM()->composites.contains(mCompName);

    if (ok){
        Composite* comp = PM()->composites[mCompName];

        // Find a unique name..
        int childIndex = 0;
        mChildName = QString("c") + QString::number(childIndex++);
        while (true){
            if (!comp->children.contains(mChildName)) break;
            else mChildName = QString("c") + QString::number(childIndex++);
        }
    }
}

void NewCompositeChildCommand::undo(){
    // Delete the child..
    Composite* comp = PM()->composites[mCompName];
    comp->children.removeAll(mChildName);
    comp->childrenMap.remove(mChildName);

    // Update widgets
    MainWindow::Instance()->compositeUpdated(mCompName);
}

void NewCompositeChildCommand::redo(){
    // Add a new blank child..
    Composite* comp = PM()->composites[mCompName];
    comp->children.append(mChildName);

    Composite::Child child;
    child.index = comp->children.count()-1;
    child.parent = -1;
    child.parentPivot = -1;
    child.part = ""; // blank
    child.z = 0;
    comp->childrenMap.insert(mChildName, child);

    // Update widgets
    MainWindow::Instance()->compositeUpdated(mCompName);
}

EditCompositeChildNameCommand::EditCompositeChildNameCommand(const QString& compName, const QString& child, const QString& newChildName)
    :mCompName(compName), mOldChildName(child), mNewChildName(newChildName){
    ok = PM()->composites.contains(mCompName);
    if (ok){
        Composite* comp = PM()->composites[mCompName];
        if (comp->children.contains(child)){
            if (comp->children.contains(mNewChildName)){
                // Find a unique variant
                int childIndex = 0;
                mNewChildName = newChildName + QString::number(childIndex++);
                while (true){
                    if (!comp->children.contains(mNewChildName)) break;
                    else mNewChildName = newChildName + QString::number(childIndex++);
                }
            }
        }
        else {
            ok = false;
        }
    }
}

void EditCompositeChildNameCommand::undo(){
    // Unchange the child name
    Composite* comp = PM()->composites[mCompName];
    comp->children.replace(comp->children.indexOf(mNewChildName), mOldChildName);
    comp->childrenMap.insert(mOldChildName, comp->childrenMap.take(mNewChildName));
    MainWindow::Instance()->compositeUpdated(mCompName);
}

void EditCompositeChildNameCommand::redo(){
    // Change the child name
    Composite* comp = PM()->composites[mCompName];
    comp->children.replace(comp->children.indexOf(mOldChildName), mNewChildName);
    comp->childrenMap.insert(mNewChildName, comp->childrenMap.take(mOldChildName));
    MainWindow::Instance()->compositeUpdated(mCompName);
}

DeleteCompositeChildCommand::DeleteCompositeChildCommand(const QString& compName, const QString& childName)
    :mCompName(compName), mChildName(childName){

    ok = PM()->composites.contains(mCompName);
    if (ok){
        Composite* comp = PM()->composites[mCompName];
        ok = comp->children.contains(childName) && comp->childrenMap.contains(childName);
        if (ok){
            mChildIndex = comp->children.indexOf(childName);
            ok = (mChildIndex>=0);
        }
    }
}

void DeleteCompositeChildCommand::undo(){
    Composite* comp = new Composite(mCompCopy);
    delete PM()->composites.take(mCompName);
    PM()->composites.insert(mCompName, comp);
    MainWindow::Instance()->compositeUpdated(mCompName);
}

int FixIndex(int i, int ci){
    if (i==ci) return -1;
    else if (i>ci) return i-1;
    else return i;
}

void DeleteCompositeChildCommand::redo(){
    Composite* comp = PM()->composites[mCompName];
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

    MainWindow::Instance()->compositeUpdated(mCompName);
}


ChangePropertiesCommand::ChangePropertiesCommand(QString partName, QString properties)
    :mPartName(partName), mProperties(properties){
    ok = PM()->parts.contains(partName);
}

void ChangePropertiesCommand::undo(){
    Part* part = PM()->parts[mPartName];
    part->properties = mOldProperties;

    MainWindow::Instance()->partPropertiesUpdated(mPartName);
}

void ChangePropertiesCommand::redo(){
    Part* part = PM()->parts[mPartName];
    mOldProperties = part->properties;
    part->properties = mProperties;

    MainWindow::Instance()->partPropertiesUpdated(mPartName);
}

ChangeCompPropertiesCommand::ChangeCompPropertiesCommand(QString compName, QString properties)
    :mCompName(compName), mProperties(properties){
    ok = PM()->composites.contains(compName);
}

void ChangeCompPropertiesCommand::undo(){
    Composite* comp = PM()->composites[mCompName];
    comp->properties = mOldProperties;

    MainWindow::Instance()->compPropertiesUpdated(mCompName);
}

void ChangeCompPropertiesCommand::redo(){
    Composite* comp = PM()->composites[mCompName];
    mOldProperties = comp->properties;
    comp->properties = mProperties;

    MainWindow::Instance()->compPropertiesUpdated(mCompName);
}

*/
