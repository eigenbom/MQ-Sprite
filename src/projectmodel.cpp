#include "projectmodel.h"

#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QImageWriter>
#include <QTemporaryFile>
#include <QDir>
#include <QtZlib/zlib.h>

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ios>

#include "tarball.h"

static ProjectModel* sInstance = NULL;

ProjectModel::ProjectModel()
{
    sInstance = this;
}

ProjectModel::~ProjectModel(){    
    clear();
}

ProjectModel* ProjectModel::Instance(){
    return sInstance;
}

void ProjectModel::clear(){

    foreach(QString key, parts.keys()){
        delete parts[key];
    }
    foreach(QString key, composites.keys())
    {
        delete composites[key];
    }
    parts.clear();
    composites.clear();
    fileName = QString();
}

void ProjectModel::loadTestData(){
    // setup temp data
    std::srand(std::time(NULL));
    static const char* PARTS[2] = {"test_foo", "bar_test"};
    static const char* MODES[2] = {"anm0", "anm9"};
    QColor colours[2] = {QColor(255,0,0), QColor(0,0,255)};

    for(int i=0;i<2;i++){
        Part* p = new Part;

        for(int j=0;j<2;j++){
            Part::Mode m;
            m.numFrames = 1;
            m.width = 16;
            m.height = 16;
            m.numPivots = 2;
            m.framesPerSecond = 24;
            for(int k=0;k<m.numFrames;k++){
                for(int p=0;p<MAX_PIVOTS;p++){
                    int px = floor((m.width-1)*(std::rand()*1.f/RAND_MAX));
                    int py = floor((m.height-1)*(std::rand()*1.f/RAND_MAX));
                    m.pivots[p].push_back(QPoint(px,py));
                }
                m.anchor.push_back(QPoint(m.width/2,m.height/2));
                QImage* img = new QImage(m.width, m.height, QImage::Format_ARGB32);
                m.images.push_back(img);
                img->fill(QColor(255,255,255,0));
                QPainter painter(img);
                painter.fillRect(m.width/4,m.height/4,m.width/2,m.height/2,colours[i]);
            }
            p->modes.insert(MODES[j],m);
            this->parts.insert(PARTS[i],p);
        }
    }

    Composite* comp = new Composite;
    comp->root = 0;

    {
        Composite::Child ch;
        ch.part = "test_foo";
        ch.index = 0;
        ch.parent = -1;
        ch.parentPivot = -1;
        ch.z = 1;
        ch.children.push_back(1);

        comp->children.push_back("parent");
        comp->childrenMap.insert(comp->children.back(), ch);
    }

    {
        Composite::Child ch;
        ch.part = "bar_test";
        ch.index = 1;
        ch.parent = 0;
        ch.parentPivot = 0;
        ch.z = 0;

        comp->children.push_back("child");
        comp->childrenMap.insert(comp->children.back(), ch);
    }

    this->composites.insert("comp", comp);
}

bool ProjectModel::load(const QString& fileName){
    std::fstream in(fileName.toStdString().c_str(), std::ios::in | std::ios::binary);
    if(!in.is_open()) qFatal("Cannot open in");

    lindenb::io::TarIn tarball(in);
    tarball.read();
    if (!tarball.ok()){
        qDebug() << "Tar not ok";
        return false;
    }

    typedef lindenb::io::TarIn::Record Record;
    typedef std::map<std::string, Record> FileMap;
    FileMap& fileMap = tarball.fileMap();

    // First check data.json version
    if (fileMap.count("data.json")==0){
        qDebug() << "No data.json in tar";
        return false;
    }
    // else ..

    const Record& dataRec = fileMap["data.json"];
    QJsonParseError error;
    QJsonDocument dataDoc = QJsonDocument::fromJson(QByteArray(dataRec.buffer, dataRec.length),&error);
    if (error.error!=QJsonParseError::NoError){
        qDebug() << error.errorString();
        return false;
    }
    else if (dataDoc.isNull() || dataDoc.isEmpty() || !dataDoc.isObject()){
        qDebug() << "data.json is empty or null or not a json object";
        return false;
    }
    // else ..

    QJsonObject dataObj = dataDoc.object();
    if (!dataObj.contains("version")){
        qDebug() << "data.json has no version";
        return false;
    }
    // else ..

    if (dataObj.value("version").toDouble(0)!=PROJECT_SAVE_FILE_VERSION){
        qDebug() << "invalid version";
        return false;
    }

    // All good!

    // TGet prefs.json (and load the settings)
    if (fileMap.count("prefs.json")>0){
        const Record& prefsRec = fileMap["prefs.json"];
        // qDebug() << "*" << QString::fromUtf8(prefsRec.buffer, prefsRec.length) << "*";

        QJsonParseError error;
        QJsonDocument prefsDoc = QJsonDocument::fromJson(QByteArray(prefsRec.buffer, prefsRec.length), &error);

        if (error.error!=QJsonParseError::NoError){
            qDebug() << error.errorString();
        }
        else if (prefsDoc.isNull() || prefsDoc.isEmpty() || !prefsDoc.isObject()){
            qDebug() << "prefs.json is empty or null or not a json object";
        }
        else {
            QSettings settings;
            QJsonObject settingsObj = prefsDoc.object();
            for(QJsonObject::iterator it = settingsObj.begin(); it!=settingsObj.end(); it++){
                const QJsonValue& val = it.value();

                if (it.key()=="background_colour"){
                    uint col = val.toString().toUInt();
                    settings.setValue(it.key(), col);
                }
                else {
                    settings.setValue(it.key(), val.toVariant());
                }
            }
        }
    }

    // Load all the images (and store them in an image map)
    QMap<QString, QImage*> imageMap;
    FileMap::iterator it = tarball.fileMap().begin();
    for(;it!=tarball.fileMap().end();it++){
        QString assetName = QString::fromStdString(it->first);
        if (assetName.endsWith(".png")){
            const Record& record = it->second;
            QImage* img = new QImage;
            bool res = img->loadFromData(QByteArray(record.buffer, record.length), "PNG");
            Q_ASSERT(res);
            imageMap.insert(assetName, img);
        }
    }

    // Load data.json, connecting the Images* too
    {
        QJsonObject parts = dataObj.value("parts").toObject();
        QJsonObject comps = dataObj.value("comps").toObject();

        if (!parts.isEmpty()){
            for(QJsonObject::iterator it = parts.begin(); it!=parts.end(); it++){
                const QString& partName = it.key();
                const QJsonObject& partObj = it.value().toObject();
                // Load the part..
                Part* part = new Part;
                part->properties = QString();
                JsonToPart(partObj, imageMap, part);
                // Add <partName, part> to mParts
                this->parts.insert(partName, part);
            }
        }

        if (!comps.isEmpty()){
            for(QJsonObject::iterator it = comps.begin(); it!=comps.end(); it++){
                const QString& compName = it.key();
                const QJsonObject& compObj = it.value().toObject();
                // Load the part..
                Composite* composite = new Composite;
                composite->properties = QString();
                JsonToComposite(compObj, composite);
                // Add <partName, part> to mParts
                this->composites.insert(compName, composite);
            }
        }
    }

    this->fileName = fileName;
    return true;
}

void ProjectModel::JsonToPart(const QJsonObject& obj, const QMap<QString,QImage*>& imageMap, Part* part){
    // foreach mode    

    for(QJsonObject::const_iterator it=obj.begin();it!=obj.end();it++){        
        const QString& modeName = it.key();

        if (modeName=="properties"){
            part->properties = it.value().toString();
            continue;
        }

        // else its a proper mode
        const QJsonObject& modeObject = it.value().toObject();
        if (!modeObject.isEmpty()){
            Part::Mode m;

            m.width = modeObject.value("width").toVariant().toInt();
            m.height = modeObject.value("height").toVariant().toInt();
            m.numFrames = modeObject.value("numFrames").toVariant().toInt();
            m.numPivots = modeObject.value("numPivots").toVariant().toInt();
            m.framesPerSecond = modeObject.value("framesPerSecond").toVariant().toInt();

            const QJsonArray& frameArray = modeObject.value("frames").toArray();
            Q_ASSERT(frameArray.count()==m.numFrames);
            for(int frame=0;frame<frameArray.count();frame++){
                const QJsonObject& frameObject = frameArray.at(frame).toObject();

                int ax = frameObject.value("ax").toVariant().toInt();
                int ay = frameObject.value("ay").toVariant().toInt();
                m.anchor.push_back(QPoint(ax,ay));

                QString imageName = frameObject.value("image").toString();
                QImage* image = imageMap.value(imageName);
                Q_ASSERT(image);
                Q_ASSERT(image->width()==m.width && image->height()==m.height);
                m.images.push_back(image);

                // pivots etc..
                for(int p=0;p<m.numPivots;p++){
                    int px = frameObject.value(QString("p%1x").arg(p)).toVariant().toInt();
                    int py = frameObject.value(QString("p%1y").arg(p)).toVariant().toInt();
                    m.pivots[p].push_back(QPoint(px,py));
                }
                for(int p=m.numPivots;p<MAX_PIVOTS;p++){
                    m.pivots[p].push_back(QPoint(0,0));
                }
            }

            part->modes.insert(modeName, m);
        }
    }
}

void ProjectModel::PartToJson(const QString& name, const Part& part, QJsonObject* obj, QMap<QString,QImage*>* imageMap){   
    if (!part.properties.isEmpty()){
        obj->insert("properties", part.properties);
    }

    QMapIterator<QString,Part::Mode> mit(part.modes);
    QString partNameFixed = name;
    partNameFixed.replace(' ', '_');

    while (mit.hasNext()){
        mit.next();

        const Part::Mode& m = mit.value();
        QString modeNameFixed = mit.key();
        modeNameFixed.replace(' ', '_');

        // part, mode..
        QJsonObject modeObject;
        modeObject.insert("width", m.width);
        modeObject.insert("height", m.height);
        modeObject.insert("numFrames", m.numFrames);
        modeObject.insert("numPivots", m.numPivots);
        modeObject.insert("framesPerSecond", m.framesPerSecond);

        QJsonArray frameArray;
        for(int frame=0;frame<m.numFrames;frame++){
            QJsonObject frameObject;
            frameObject.insert("ax", m.anchor.at(frame).x());
            frameObject.insert("ay", m.anchor.at(frame).y());

            QString frameNum = QString("%1").arg(frame, 3, 10, QChar('0')).toUpper();
            QString imageName = QString("%1_%2_%3.png").arg(partNameFixed,modeNameFixed,frameNum);

            imageMap->insert(imageName, m.images.at(frame));
            frameObject.insert("image", imageName);

            // pivots
            for(int p=0;p<m.numPivots;p++){
                frameObject.insert(QString("p%1x").arg(p), m.pivots[p].at(frame).x());
                frameObject.insert(QString("p%1y").arg(p), m.pivots[p].at(frame).y());
            }

            frameArray.append(frameObject);
        }

        modeObject.insert("frames", frameArray);
        obj->insert(mit.key(), modeObject);
    }
}

void ProjectModel::CompositeToJson(const QString&, const Composite& comp, QJsonObject* obj){
    obj->insert("root", comp.root);
    obj->insert("properties", comp.properties);

    QJsonArray compChildren;
    foreach(const QString& childName, comp.children){
        QString fixedChildName = childName;
        fixedChildName.replace(' ','_');

        const Composite::Child& child = comp.childrenMap.value(childName);
        QJsonObject childObject;
        childObject.insert("name", fixedChildName);
        childObject.insert("parent", child.parent);
        childObject.insert("parentPivot", child.parentPivot);
        childObject.insert("z", child.z);
        childObject.insert("part", child.part);

        QJsonArray children;
        foreach(int ci, child.children){
            children.append(ci);
        }
        childObject.insert("children", children);

        compChildren.push_back(childObject);
        // compChildren.insert(fixedChildName, childObject);
    }
    obj->insert("parts", compChildren);
}

void ProjectModel::JsonToComposite(const QJsonObject& obj, Composite* comp){
    comp->root = obj.value("root").toVariant().toInt();

    comp->properties = obj.value("properties").toString();

    QJsonArray children = obj.value("parts").toArray();
    int index = 0;
    foreach(const QJsonValue& value, children){
        QJsonObject childObject = value.toObject();
        QString name = childObject.value("name").toString();
        comp->children.push_back(name);

        Composite::Child child;
        child.parent = childObject.value("parent").toVariant().toInt();
        child.parentPivot = childObject.value("parentPivot").toVariant().toInt();
        child.z = childObject.value("z").toVariant().toInt();
        child.part = childObject.value("part").toString();
        child.index = index;

        QJsonArray childrenOfChild = childObject.value("children").toArray();
        foreach(const QJsonValue& ci, childrenOfChild){
            child.children.push_back(ci.toVariant().toInt());
        }
        comp->childrenMap.insert(name, child);

        index++;
    }
}

bool ProjectModel::save(const QString& fileName){
    std::fstream out(fileName.toStdString().c_str(), std::ios::out | std::ios::binary);
    if(!out.is_open()){
        qDebug() << "Cannot open out";
        return false;
    }
    QMap<QString,QImage*> imageMap;

    lindenb::io::TarOut tarball(out);

    //////////////////////
    // data.json
    // NB: build the image set while processing data
    //////////////////////

    {
        QJsonObject data;

        // version etc
        data.insert("version", PROJECT_SAVE_FILE_VERSION);
        // data.insert("test", QString("booo"));

        // parts
        QJsonObject partsObject;

        QMapIterator<QString,Part*> pit(this->parts);
        while (pit.hasNext()){
            pit.next();
            QJsonObject partObject;
            const Part* p = pit.value();
            PartToJson(pit.key(), *p, &partObject, &imageMap);
            partsObject.insert(pit.key(), partObject);
        }
        data.insert("parts", partsObject);

        // comps
        QJsonObject compsObject;
        {
            QMapIterator<QString, Composite*> it(this->composites);
            while (it.hasNext()){
                it.next();
                QString compNameFixed = it.key();
                compNameFixed.replace(' ','_');
                QJsonObject compObject;
                const Composite* comp = it.value();
                CompositeToJson(compNameFixed, *comp, &compObject);
                compsObject.insert(it.key(), compObject);
            }
        }
        data.insert("comps", compsObject);

        // Send it out
        QString dataStr;
        QTextStream out(&dataStr);
        QJsonDocument doc(data);
        out << doc.toJson();
        tarball.put("data.json", dataStr.toStdString().c_str());
    }

    //////////////////////
    // prefs.json
    //////////////////////

    {
        QJsonObject settingsObj;
        QSettings settings;
        foreach(QString key, settings.allKeys()){
            const QVariant& val = settings.value(key);
            if (key=="background_colour"){
                uint col = val.toUInt();
                settingsObj.insert(key, QString::number(col));
            }
            else {
                settingsObj.insert(key, QJsonValue::fromVariant(val));
            }
        }

        QString prefsdataStr;
        QTextStream prefsout(&prefsdataStr);
        QJsonDocument prefsdoc(settingsObj);
        prefsout << prefsdoc.toJson();

        tarball.put("prefs.json", prefsdataStr.toStdString().c_str());
    }

    ////////////////
    // images.json
    ////////////////

    {
        QDir tempPath = QDir(QDir::tempPath());
        QString tempFileName = tempPath.absoluteFilePath("tmp.png");

        QMapIterator<QString,QImage*> it(imageMap);
        while (it.hasNext()){
            it.next();

            const QImage* img = it.value();
            if (img){
                bool res = img->save(tempFileName, "PNG");
                if (res){
                    tarball.putFile(tempFileName.toStdString().c_str(),it.key().toStdString().c_str());
                }
                else {
                    qDebug() << "Couldn't save image: " << it.key();
                }
            }
        }
    }

    tarball.finish();
    out.close();

    this->fileName = fileName;
    return true;

    /*
    // gzip utility functions from: http://zlib.net/manual.html#Utility
    gzFile gz = gzopen(zipFileName.toStdString().c_str(), "wbT"); // "T" no compression
    const char* buf = "Hello";
    gzwrite(gz, buf, strlen(buf));
    gzclose(gz);
    */
}

Part::~Part(){
    foreach(QString key, modes.keys()){
        Part::Mode& m = modes[key];
        for(int i=0;i<m.numFrames;i++){
            if (i<m.images.size()){
                QImage* img = m.images[i];
                if (img) delete img;
            }
        }
    }
}
