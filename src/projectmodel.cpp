#include "projectmodel.h"

#include "zip.h"
#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDir>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ios>

static const int ProjectFileVersion = 2;

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

ProjectModel::~ProjectModel() {
	clear();
}

ProjectModel* ProjectModel::Instance() {
	return sInstance;
}

AssetRef ProjectModel::createAssetRef(AssetType type) {
	AssetRef ref;
	ref.id = mNextId++;
	ref.type = type;
	return ref;
}

Asset* ProjectModel::getAsset(const AssetRef& ref) {
	switch (ref.type) {
	case AssetType::Part: return getPart(ref);
	case AssetType::Composite: return getComposite(ref);
	case AssetType::Folder: return getFolder(ref);
	}
	return nullptr;
}

bool ProjectModel::hasAsset(const AssetRef& ref) {
	return getAsset(ref) != nullptr;
}

Part* ProjectModel::getPart(const AssetRef& uuid) {
	return parts.value(uuid).data();
}

bool ProjectModel::hasPart(const AssetRef& uuid) {
	return getPart(uuid) != nullptr;
}

Composite* ProjectModel::getComposite(const AssetRef& uuid) {
	return composites.value(uuid).data();
}

bool ProjectModel::hasComposite(const AssetRef& uuid) {
	return getComposite(uuid) != nullptr;
}

Folder* ProjectModel::getFolder(const AssetRef& uuid) {
	return folders.value(uuid).data();
}

bool ProjectModel::hasFolder(const AssetRef& uuid) {
	return getFolder(uuid) != nullptr;
}

Part* ProjectModel::findPartByName(const QString& name) {
	for (auto p : parts.values()) {
		if (p->name == name) {
			return p.data();
		}
	}
	return nullptr;
}

Composite* ProjectModel::findCompositeByName(const QString& name) {
	for (auto p : composites.values()) {
		if (p->name == name) {
			return p.data();
		}
	}
	return nullptr;
}

Folder* ProjectModel::findFolderByName(const QString& name) {
	for (auto f : folders.values()) {
		if (f->name == name) {
			return f.data();
		}
	}
	return nullptr;
}

void ProjectModel::clear() {
	parts.clear();
	composites.clear();
	folders.clear();
	fileName = QString();
	mNextId = 0;
}

static void removeAdditionalNullChars(QByteArray& arr) {
	int length = 0;
	for (int i = 0; i < arr.length(); ++i) {
		if (arr[i] == '\0') {
			length = i;
			break;
		}
	}
	if (length != 0) arr.resize(length);
}

bool ProjectModel::load(const QString& fileName, QString& reason) {
	importLog.clear();
	mNextId = 0;

	auto fileMap = LoadZip(fileName);
	if (fileMap.isEmpty()) {
		reason = "Cannot open file!";
		return false;
	}

	if (fileMap.count("data.json") == 0) {
		reason = "Internal data.json is missing";
		return false;
	}

	auto& dataRec = fileMap["data.json"];
	// qDebug() << "data.json: " << dataRec;
	removeAdditionalNullChars(dataRec);
	// qDebug() << "data.json: " << dataRec;

	QJsonParseError error;
	QJsonDocument dataDoc = QJsonDocument::fromJson(dataRec, &error);

	auto buildErrorString = [&](QString reason) -> QString {
		QStringList list { reason + ": " + error.errorString() };
		if (error.offset >= 0 && error.offset < dataRec.length()) {
			auto start = std::next(dataRec.data() + std::max(0, error.offset - 20));
			auto end = std::next(dataRec.data() + std::min(dataRec.length() - 1, error.offset + 20));
			if (start < end) {
				list.append("Context: ");
				list.append(QString::fromUtf8(start, std::distance(start, end)));
			}
		}
		return list.join("\n");
	};

	if (error.error != QJsonParseError::NoError) {
		reason = buildErrorString("Internal data.json parse error");
		return false;
	}
	else if (dataDoc.isNull() || dataDoc.isEmpty() || !dataDoc.isObject()) {
		reason = buildErrorString("Internal data.json is not a valid json object");
		return false;
	}

	QJsonObject dataObj = dataDoc.object();
	if (!dataObj.contains("version")) {
		reason = buildErrorString("Internal data.json has no version field");
		return false;
	}

	if (dataObj.value("version").toInt(0) != ProjectFileVersion) {
		reason = buildErrorString("Internal data.json has an invalid version");
		return false;
	}

	// Load all the images (and store them in an image map)
	// The ownership of these are taken by the sprites when they're loaded
	QMap<QString, QSharedPointer<QImage>> imageMap;
	
	for (auto it = fileMap.begin(); it != fileMap.end(); it++) {
		QString assetName = it.key();
		if (assetName.endsWith(".png")) {
			auto img = QSharedPointer<QImage>::create();
			bool res = img->loadFromData(it.value(), "PNG");
			if (!res) {
				reason = "Couldn't load " + it.value();
				return false;
			}
			imageMap.insert(assetName, img);
		}
	}

	auto folders = dataObj.value("folders").toArray();
	auto parts = dataObj.value("parts").toArray();
	auto comps = dataObj.value("comps").toArray();

	if (!folders.isEmpty()) {
		for (auto it = folders.begin(); it != folders.end(); it++) {
			const auto& obj = it->toObject();
			auto folder = QSharedPointer<Folder>::create();
			folder->ref.id = obj["id"].toInt();
			folder->ref.type = AssetType::Folder;
			mNextId = std::max(mNextId, folder->ref.id + 1);
			jsonToFolder(obj, folder.get());
			this->folders.insert(folder->ref, folder);
		}
	}

	if (!parts.isEmpty()) {
		for (auto it = parts.begin(); it != parts.end(); it++) {
			const auto& partObj = it->toObject();
			auto part = QSharedPointer<Part>::create();
			part->ref.id = partObj["id"].toInt();
			part->ref.type = AssetType::Part;
			mNextId = std::max(mNextId, part->ref.id + 1);
			jsonToPart(partObj, imageMap, part.get());
			this->parts.insert(part->ref, part);
		}
	}

	if (!comps.isEmpty()) {
		for (auto it = comps.begin(); it != comps.end(); it++) {
			const auto& compObj = it->toObject();
			auto composite = QSharedPointer<Composite>::create();
			composite->ref.id = compObj["id"].toInt();
			composite->ref.type = AssetType::Composite;
			mNextId = std::max(mNextId, composite->ref.id + 1);
			jsonToComposite(compObj, composite.get());
			this->composites.insert(composite->ref, composite);
		}
	}

	this->fileName = fileName;
	return true;
}

bool ProjectModel::save(const QString& fileName) {
	QMap<QString, QSharedPointer<QImage>> imageMap;
	QMap<QString, QString> fileMap;

	QList<QString> tempFiles;

	{
		QJsonObject data;
		data.insert("version", ProjectFileVersion);

		QJsonArray foldersArray;
		for (auto folder : folders) {
			QJsonObject folderObject;
			folderObject.insert("id", folder->ref.id);
			folderToJson(folder->name, *folder, &folderObject);
			foldersArray.append(folderObject);
		}
		data.insert("folders", foldersArray);

		QJsonArray partsArray;
		for (auto part : parts) {
			QJsonObject partObject;
			partObject.insert("id", part->ref.id);
			partToJson(part->name, *part, &partObject, &imageMap);
			partsArray.append(partObject);
		}
		data.insert("parts", partsArray);

		QJsonArray compArray;
		for (auto comp: composites) {
			QJsonObject compObject;
			compObject.insert("id", comp->ref.id);
			compositeToJson(comp->name, *comp, &compObject);
			compArray.append(compObject);
		}
		data.insert("comps", compArray);

		QString pathTemplate = QDir(QDir::tempPath()).absoluteFilePath("data.XXXXXX.json");
		QTemporaryFile file(pathTemplate);
		if (!file.open()){
			exportLog.append("Couldn't create temporary file " + pathTemplate);
			return false;
		}
		file.setAutoRemove(false);
		tempFiles.append(file.fileName());

		QTextStream out(&file);
		QJsonDocument doc(data);
		out << doc.toJson();
		fileMap.insert("data.json", file.fileName());
	}

	{
		for (auto it = imageMap.begin(); it != imageMap.end(); ++it) {
			auto img = it.value();
			if (img) {
				auto imageName = it.key();
				imageName.replace(' ', '_');

				QString pathTemplate = QDir(QDir::tempPath()).absoluteFilePath(imageName + "-XXXXXX.png");
				QTemporaryFile file(pathTemplate);				
				if (!file.open()) {
					exportLog.append("Couldn't create temporary file: " + pathTemplate);
					return false;
				}

				file.setAutoRemove(false);
				tempFiles.append(file.fileName());
				bool res = img->save(&file, "PNG");
				if (res) {
					fileMap.insert(it.key(), file.fileName());
				}
				else {
					exportLog.append("Couldn't save image " + it.key());
				}
			}
		}
	}

	{
		QDir tempPath = QDir(QDir::tempPath());
		QString tempFileName = tempPath.absoluteFilePath("tmp.mqs");
		bool success = WriteZip(tempFileName, fileMap);
		if (!success) {
			exportLog.append("Couldn't write zip!");
			return false;
		}

		// Copy tempFileName to fileName
		if (QFile::exists(fileName)){
			QFile::remove(fileName);
		}
		QFile::copy(tempFileName, fileName);	
	}

	for (auto file : tempFiles) {
		QFile::remove(file);
	}	
	
	this->fileName = fileName;
	return true;
}

void ProjectModel::jsonToFolder(const QJsonObject& obj, Folder* folder){
    folder->name = obj["name"].toString();
    if (obj.contains("parent")){
		folder->parent.id = obj["parent"].toInt();
        folder->parent.type = AssetType::Folder;
    }
}

void ProjectModel::folderToJson(const QString& name, const Folder& folder, QJsonObject* obj){

    obj->insert("name", name);
    if (!folder.parent.isNull()){
        obj->insert("parent", folder.parent.id);
    }
}

void ProjectModel::jsonToPart(const QJsonObject& obj, const QMap<QString,QSharedPointer<QImage>>& imageMap, Part* part){
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

void ProjectModel::partToJson(const QString& name, const Part& part, QJsonObject* obj, QMap<QString, QSharedPointer<QImage>>* imageMap){
    auto properties = part.properties.trimmed();
    if (!properties.isEmpty()){
        obj->insert("properties", "{ " + properties + " }");
    }

    QString imageNamePrefix = name;
	imageNamePrefix.append(" " + QString::number(part.ref.id)); // Append id to ensure uniqueness
	if (!part.parent.isNull()) {
		Q_ASSERT(getFolder(part.parent) != nullptr);
		QStringList list;
		BuildFolderList(this, *getFolder(part.parent), list);		
		imageNamePrefix.prepend(list.join("_").append("_"));
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

void ProjectModel::compositeToJson(const QString& name, const Composite& comp, QJsonObject* obj){
    obj->insert("root", comp.root);
    obj->insert("name", name);

	auto properties = comp.properties.trimmed();
	if (!properties.isEmpty()) {
		obj->insert("properties", "{ " + properties + " }");
	}

    if (!comp.parent.isNull()){
        obj->insert("parent", comp.parent.id);
    }

    QJsonArray compChildren;
	int index = 0;
    for(const auto& childName: comp.children){
        const auto& child = comp.childrenMap.value(childName);
        QJsonObject childObject;
		childObject.insert("id", child.id); // Unused
        childObject.insert("name", childName);
        childObject.insert("parent", child.parent);
        childObject.insert("parentPivot", child.parentPivot);
        childObject.insert("z", child.z);
		childObject.insert("index", index++);
        childObject.insert("part", child.part.id);
        QJsonArray children;
        for(int ci: child.children){
            children.append(ci);
        }
        childObject.insert("children", children);
        compChildren.push_back(childObject);
    }
    obj->insert("parts", compChildren);
}

void ProjectModel::jsonToComposite(const QJsonObject& obj, Composite* comp){
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
        child.index = index++;
        QJsonArray childrenOfChild = childObject.value("children").toArray();
        for(const auto& ci: childrenOfChild){
            child.children.push_back(ci.toInt());
        }
        comp->childrenMap.insert(name, child);

		if (childObject.contains("index")) {
			int childIndex = childObject.value("index").toInt();
			if (childIndex != child.index) {
				importLog.append("Index of child of " + name + " is incorrect!");
			}
		}		
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