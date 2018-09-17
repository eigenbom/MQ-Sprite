#include "projectmodel.h"

#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
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

static const int PROJECT_SAVE_FILE_VERSION = 2;

bool operator==(const AssetRef& a, const AssetRef& b){
    return (a.type == AssetType::None && b.type == AssetType::None) ||  (a.id == b.id && a.type == b.type);
}

bool operator!=(const AssetRef& a, const AssetRef& b){
    return !(a==b);
}

bool operator<(const AssetRef& a, const AssetRef& b){
    return a.id > b.id;
}

uint qHash(const AssetRef &key){
    return qHash(std::make_pair(key.id, (int) key.type));
}

Preferences& GlobalPreferences() {
	static Preferences prefs;
	return prefs;
}

static ProjectModel* sInstance = nullptr;

ProjectModel* PM(){return ProjectModel::Instance();}

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

AssetRef ProjectModel::createAssetRef(AssetType type){
    AssetRef ref;
	ref.id = mNextId++;
	ref.type = type;
    return ref;
}

Asset* ProjectModel::getAsset(const AssetRef& ref){
    switch (ref.type){
        case AssetType::Part: return getPart(ref);
        case AssetType::Composite: return getComposite(ref);
        case AssetType::Folder: return getFolder(ref);
    }
    return nullptr;
}

bool ProjectModel::hasAsset(const AssetRef& ref){
    return getAsset(ref)!=nullptr;
}

Part* ProjectModel::getPart(const AssetRef& uuid){
    return parts.value(uuid).data();
}


bool ProjectModel::hasPart(const AssetRef& uuid){
    return getPart(uuid)!=nullptr;
}

Composite* ProjectModel::getComposite(const AssetRef& uuid){
    return composites.value(uuid).data();
}

bool ProjectModel::hasComposite(const AssetRef& uuid){
    return getComposite(uuid)!=nullptr;
}

Folder* ProjectModel::getFolder(const AssetRef& uuid){
    return folders.value(uuid).data();
}

bool ProjectModel::hasFolder(const AssetRef& uuid){
    return getFolder(uuid)!=nullptr;
}


Part* ProjectModel::findPartByName(const QString& name){
    for(auto p: parts.values()){
        if (p->name==name){
            return p.data();
        }
    }
    return nullptr;
}

Composite* ProjectModel::findCompositeByName(const QString& name){
    for(auto p: composites.values()){
        if (p->name==name){
            return p.data();
        }
    }
    return nullptr;
}

Folder* ProjectModel::findFolderByName(const QString& name){
    for(auto f: folders.values()){
        if (f->name==name){
            return f.data();
        }
    }
    return nullptr;
}

void ProjectModel::clear(){
    parts.clear();
    composites.clear();
    folders.clear();
    fileName = QString();
	mNextId = 0;
}

bool ProjectModel::load(const QString& fileName, QString& reason){	
	importLog.clear();
	mNextId = 0;

    std::fstream in(fileName.toStdString().c_str(), std::ios::in | std::ios::binary);
	if (!in.is_open()) {
		reason = "Cannot open file";
		return false;
	}

    lindenb::io::TarIn tarball(in);
    tarball.read();
    if (!tarball.ok()){
		reason = "Cannot read project file";
        return false;
    }

    auto& fileMap = tarball.fileMap();

    if (fileMap.count("data.json") == 0){
		reason = "Internal data.json is missing";
        return false;
    }

    auto& dataRec = fileMap["data.json"];

	int dataLength = 0;
	for (int i = 0; i < dataRec.length; ++i) {
		if (dataRec.buffer[i] == '\0') {
			dataLength = i;
			break;
		}
	}

	if (dataLength == 0) {
		reason = "Internal data.json is empty";
		return false;
	}

    QJsonParseError error;
    QJsonDocument dataDoc = QJsonDocument::fromJson(QByteArray(dataRec.buffer, dataLength), &error);
	
    if (error.error != QJsonParseError::NoError){		
		reason = "Internal data.json parse error: " + error.errorString();		
        return false;
    }
    else if (dataDoc.isNull() || dataDoc.isEmpty() || !dataDoc.isObject()){
		reason = "Internal data.json is not a valid json object";
        return false;
    }

    QJsonObject dataObj = dataDoc.object();
    if (!dataObj.contains("version")){
		reason = "Internal data.json has no version field";
        return false;
    }

    if (dataObj.value("version").toInt(0) != PROJECT_SAVE_FILE_VERSION){
		reason = "Internal data.json has an invalid version";
        return false;
    }

	qDebug() << "TODO: Serialise preferences";

	/*
    if (fileMap.count("prefs.json")>0){
        auto& prefsRec = fileMap["prefs.json"];
		int presLength = 0;

		for (int i = 0; i < prefsRec.length; ++i) {
			if (prefsRec.buffer[i] == '\0') {
				presLength = i;
				break;
			}
		}

        QJsonParseError error;
        QJsonDocument prefsDoc = QJsonDocument::fromJson(QByteArray(prefsRec.buffer, presLength), &error);

        if (error.error!=QJsonParseError::NoError){
            qWarning() << "Internal prefs.json parse error: " << error.errorString();
        }
        else if (prefsDoc.isNull() || prefsDoc.isEmpty() || !prefsDoc.isObject()){
			qWarning() << "Internal prefs.json is not a vaid json object";
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
	*/

    // Load all the images (and store them in an image map)
	// The ownership of these are taken by the sprites when they're loaded
    QMap<QString, QSharedPointer<QImage>> imageMap;   
    for(auto it = tarball.fileMap().begin(); it!=tarball.fileMap().end(); it++){
        QString assetName = QString::fromStdString(it->first);
        if (assetName.endsWith(".png")){
            const auto& record = it->second;
            auto img = QSharedPointer<QImage>::create();
            bool res = img->loadFromData(QByteArray(record.buffer, record.length), "PNG");
			// qDebug() << "Loaded: " << assetName << " " << img->width() << " x " << img->height();
            Q_ASSERT(res);
            imageMap.insert(assetName, img);
        }
    }

    // Load data.json, connecting the Images* too
    {
		auto folders = dataObj.value("folders").toArray();
		auto parts = dataObj.value("parts").toArray();
		auto comps = dataObj.value("comps").toArray();
		
        if (!folders.isEmpty()){
            for(auto it = folders.begin(); it!=folders.end(); it++){
                const auto& obj = it->toObject();
                auto folder = QSharedPointer<Folder>::create();
                folder->ref.id = obj["id"].toInt();
                folder->ref.type = AssetType::Folder;
				mNextId = std::max(mNextId, folder->ref.id + 1);
                JsonToFolder(obj, folder.get());
                this->folders.insert(folder->ref, folder);
            }
        }

        if (!parts.isEmpty()){
            for(auto it = parts.begin(); it!=parts.end(); it++){
                const auto& partObj = it->toObject();
				auto part = QSharedPointer<Part>::create();
				part->ref.id = partObj["id"].toInt();
                part->ref.type = AssetType::Part;
				mNextId = std::max(mNextId, part->ref.id + 1);
                JsonToPart(partObj, imageMap, part.get());
                this->parts.insert(part->ref, part);
            }
        }

        if (!comps.isEmpty()){
            for(auto it = comps.begin(); it!=comps.end(); it++){
                const auto& compObj = it->toObject();
				auto composite = QSharedPointer<Composite>::create();
				composite->ref.id = compObj["id"].toInt();
                composite->ref.type = AssetType::Composite;
				mNextId = std::max(mNextId, composite->ref.id + 1);
                JsonToComposite(compObj, composite.get());
                this->composites.insert(composite->ref, composite);
            }
        }
    }

    this->fileName = fileName;
    return true;
}

bool ProjectModel::save(const QString& fileName){
    std::ofstream out(fileName.toStdString().c_str(), std::ios::out | std::ios::binary);
    if(!out.is_open()){
		exportLog.push_back("Cannot open " + fileName);
        return false;
    }

    QMap<QString, QSharedPointer<QImage>> imageMap;

    lindenb::io::TarOut tarball(out);
	
    {
        QJsonObject data;
        data.insert("version", PROJECT_SAVE_FILE_VERSION);

        // folder
        QJsonArray foldersArray;
        for (auto folder: folders){
            QJsonObject folderObject;
			folderObject.insert("id", folder->ref.id);			
            FolderToJson(folder->name, *folder, &folderObject);
            foldersArray.append(folderObject);
        }
        data.insert("folders", foldersArray);

        // parts
		QJsonArray partsArray;
		for (auto part: parts) {
            QJsonObject partObject;
			partObject.insert("id", part->ref.id);
            PartToJson(part->name, *part, &partObject, &imageMap);
			partsArray.append(partObject);
        }
        data.insert("parts", partsArray);

		/*
        // comps
        QJsonObject compsObject;
        {
            QMapIterator<AssetRef, Composite*> it(this->composites);
            while (it.hasNext()){
                it.next();
                const Composite* comp = it.value();
                QString compNameFixed = comp->name;
                compNameFixed.replace(' ','_');
                QJsonObject compObject;
                CompositeToJson(compNameFixed, *comp, &compObject);

                QString uuid = comp->ref.uuid.toString();
                compsObject.insert(uuid, compObject);
            }
        }
        data.insert("comps", compsObject);
		*/

        // Send it out
        QString dataStr;
        QTextStream out(&dataStr);
        QJsonDocument doc(data);
        out << doc.toJson();
        tarball.put("data.json", dataStr.toStdString().c_str());
    }

    ////////////////
    // images.json
    ////////////////

    {
        QDir tempPath = QDir(QDir::tempPath());
        QString tempFileName = tempPath.absoluteFilePath("tmp.png");

		for (auto it = imageMap.begin(); it != imageMap.end(); ++it){
            auto img = it.value();
            if (img){
                bool res = img->save(tempFileName, "PNG");
                if (res){
                    tarball.putFile(tempFileName.toStdString().c_str(), it.key().toStdString().c_str());
                }
                else {
					exportLog.append("Couldn't save image " + it.key());
                }
            }
        }
    }

    tarball.finish();
    out.close();

    this->fileName = fileName;
    return true;
}

void ProjectModel::JsonToFolder(const QJsonObject& obj, Folder* folder){
    folder->name = obj["name"].toString();
    if (obj.contains("parent")){
		folder->parent.id = obj["parent"].toInt();
        folder->parent.type = AssetType::Folder;
    }
}

void ProjectModel::FolderToJson(const QString& name, const Folder& folder, QJsonObject* obj){

    obj->insert("name", name);
    if (!folder.parent.isNull()){
        obj->insert("parent", folder.parent.id);
    }
}


void ProjectModel::JsonToPart(const QJsonObject& obj, const QMap<QString,QSharedPointer<QImage>>& imageMap, Part* part){
	part->name = obj["name"].toString();

    if (obj.contains("parent")){
		part->parent.id = obj["parent"].toInt();
        part->parent.type = AssetType::Folder;
    }

	if (obj.contains("properties")) {
		part->properties = importAndFormatProperties(part->name, obj["properties"].toString());
	}

	const auto& modeArray = obj["modes"].toArray();
    for(auto it = modeArray.begin(); it != modeArray.end(); it++){
		const auto& modeObject = it->toObject();
        if (!modeObject.isEmpty()){
			const QString& modeName = modeObject["name"].toString();

            Part::Mode m;

            m.width = modeObject.value("width").toInt();
            m.height = modeObject.value("height").toInt();
            m.numFrames = modeObject.value("numFrames").toInt();
            m.numPivots = modeObject.value("numPivots").toInt();
            m.framesPerSecond = modeObject.value("framesPerSecond").toInt();
			
            const QJsonArray& frameArray = modeObject.value("frames").toArray();
            Q_ASSERT(frameArray.count() == m.numFrames);
            for(int frame=0;frame<frameArray.count();frame++){
                const QJsonObject& frameObject = frameArray.at(frame).toObject();

                int ax = frameObject.value("ax").toInt();
                int ay = frameObject.value("ay").toInt();
                m.anchor.push_back(QPoint(ax, ay));

                QString imageName = frameObject.value("image").toString();
                auto image = imageMap.value(imageName);
                Q_ASSERT(image);

				int imageHeight = image->height();
				int imageWidth = image->width();

				if (imageWidth != m.width || imageHeight != m.height) {
					importLog.append("Sprite " + part->name + " had invalid width and height! Anchors may be incorrect!");
					m.width = imageWidth;
					m.height = imageHeight;
				}

				m.frames.push_back(image);
                for(int p=0;p<m.numPivots;p++){
                    int px = frameObject.value(QString("p%1x").arg(p)).toInt();
                    int py = frameObject.value(QString("p%1y").arg(p)).toInt();
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

void BuildFolderList(ProjectModel* pm, Folder& folder, QStringList& list) {
	if (!folder.parent.isNull()) {
		Q_ASSERT(pm->getFolder(folder.parent) != nullptr);
		BuildFolderList(pm, *pm->getFolder(folder.parent), list);
	}

	list.append(folder.name);
}

void ProjectModel::PartToJson(const QString& name, const Part& part, QJsonObject* obj, QMap<QString, QSharedPointer<QImage>>* imageMap){
    auto properties = part.properties.trimmed();
    if (!properties.isEmpty()){
        obj->insert("properties", "{ " + properties + " }");
    }

    QString imageNamePrefix = name;
	imageNamePrefix.append(" " + part.ref.id); // Append id to ensure uniqueness
	if (!part.parent.isNull()) {
		Q_ASSERT(getFolder(part.parent) != nullptr);
		QStringList list;
		BuildFolderList(this, *getFolder(part.parent), list);		
		imageNamePrefix.prepend(list.join("_"));
		imageNamePrefix.append("_");
	}
	imageNamePrefix.replace(' ', '_');

    obj->insert("name", name);

    if (!part.parent.isNull()){
        obj->insert("parent", part.parent.id);
    }

	QJsonArray modeArray;
	for (auto mit = part.modes.begin(); mit != part.modes.end(); ++mit) {
		const auto& m = mit.value();
		QString modeNameFixed = mit.key();
		modeNameFixed.replace(' ', '_');

		QJsonObject modeObject;
		modeObject.insert("name", mit.key());
		modeObject.insert("width", m.width);
		modeObject.insert("height", m.height);
		modeObject.insert("numFrames", m.numFrames);
		modeObject.insert("numPivots", m.numPivots);
		modeObject.insert("framesPerSecond", m.framesPerSecond);

		QJsonArray frameArray;
		for (int frame = 0; frame < m.numFrames; frame++) {
			QJsonObject frameObject;
			frameObject.insert("ax", m.anchor.at(frame).x());
			frameObject.insert("ay", m.anchor.at(frame).y());
			QString frameNum = QString("%1").arg(frame, 3, 10, QChar('0')).toUpper();
			QString imageName = QString("%1_%2_%3.png").arg(imageNamePrefix, modeNameFixed, frameNum);
			imageMap->insert(imageName, m.frames.at(frame));
			frameObject.insert("image", imageName);
			for (int p = 0; p < m.numPivots; p++) {
				frameObject.insert(QString("p%1x").arg(p), m.pivots[p].at(frame).x());
				frameObject.insert(QString("p%1y").arg(p), m.pivots[p].at(frame).y());
			}
			frameArray.append(frameObject);
		}

		modeObject.insert("frames", frameArray);
		modeArray.append(modeObject);
	}
	obj->insert("modes", modeArray);
}

void ProjectModel::CompositeToJson(const QString& name, const Composite& comp, QJsonObject* obj){
	/*
    obj->insert("root", comp.root);
    obj->insert("name", name);

	// TODO: Reformat the properties
	qWarning() << "TODO: Reformat comp properties";
    obj->insert("properties", comp.properties);

    if (!comp.parent.isNull()){
        obj->insert("parent", comp.parent.idAsString());
    }

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
        childObject.insert("part", child.part.idAsString());

        QJsonArray children;
        foreach(int ci, child.children){
            children.append(ci);
        }
        childObject.insert("children", children);

        compChildren.push_back(childObject);
        // compChildren.insert(fixedChildName, childObject);
    }
    obj->insert("parts", compChildren);
	*/
}

void ProjectModel::JsonToComposite(const QJsonObject& obj, Composite* comp){
    comp->root = obj.value("root").toInt();    
    comp->name = obj["name"].toString();

	if (obj.contains("properties")) {
		comp->properties = importAndFormatProperties(comp->name, obj["properties"].toString());
	}

    if (obj.contains("parent")){
		comp->parent.id = obj["parent"].toInt();
        comp->parent.type = AssetType::Folder;
    }

    const auto& children = obj.value("parts").toArray();
    int index = 0;
    for(const auto& value: children){
        QJsonObject childObject = value.toObject();
        QString name = childObject.value("name").toString();
        comp->children.push_back(name);

        Composite::Child child;
		child.id = childObject.value("id").toInt();
        child.parent = childObject.value("parent").toInt();
        child.parentPivot = childObject.value("parentPivot").toInt();
        child.z = childObject.value("z").toInt();
		child.part.id = childObject.value("part").toInt();
        child.part.type = AssetType::Part;
        child.index = index;

        QJsonArray childrenOfChild = childObject.value("children").toArray();
        for(const auto& ci: childrenOfChild){
            child.children.push_back(ci.toInt());
        }
        comp->childrenMap.insert(name, child);

        index++;
    }
}

QString ProjectModel::importAndFormatProperties(const QString& assetName, const QString& properties_) {
	auto properties = properties_;
	properties = properties.trimmed();
	if (!properties.isEmpty()) {
		auto doc = QJsonDocument::fromJson(properties.toUtf8());
		if (doc.isNull()) {
			importLog.push_back("Error reading properties of " + assetName + " (Couldn't parse JSON)");
		}
		else if (!doc.isObject()) {
			importLog.push_back("Error reading properties of " + assetName + " (Not a JSON object)");
		}
		else {
			properties = QString(doc.toJson(QJsonDocument::JsonFormat::Indented));
			int start = properties.indexOf("{");
			int end = properties.lastIndexOf("}");
			if (start == -1 || end == -1) {
				importLog.push_back("Properties of " + assetName + " were not surrounded by curly braces!");
			}
			properties = properties.mid(start + 1, end - (start + 1));
			auto plist = properties.split("\n");
			for (auto& s : plist) {
				if (s.startsWith("    ")) s = s.mid(4);
			}
			properties = plist.join("\n").trimmed();
		}
	}
	return properties;
}