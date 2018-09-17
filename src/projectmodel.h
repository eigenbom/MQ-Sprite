#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <QList>
#include <QImage>
#include <QMap>
#include <QString>
#include <QPoint>
#include <QJsonObject>
#include <QSharedPointer>

static const int MAX_PIVOTS = 4;
static const int MAX_CHILDREN = 4;

struct Asset;
struct Part;
struct Composite;
struct Folder;

struct Preferences {
	QColor backgroundColour { 255, 255, 255, 255 };
	bool backgroundCheckerboard = true;
	bool showAnchors = false;

	bool showDropShadow = true;
	QColor dropShadowColour { 0, 0, 0, 255 };
	float dropShadowOpacity = 0.4f;
	float dropShadowBlurRadius = 2.0f;
	float dropShadowOffsetH = 0.2f;
	float dropShadowOffsetV = 0.3f;

	bool showOnionSkinning = false;
	float onionSkinningOpacity = 0.2f;
};

Preferences& GlobalPreferences();

enum class AssetType {
	None = 0,
    Part = 1,
    Composite = 2,
    Folder = 3,
};

struct AssetRef {
	int id { 0 }; // A unique ID for this project file
    AssetType type = AssetType::None;
	bool isNull() const { return type == AssetType::None; }
	QString idAsString() const {
		QString str;
		str.setNum(id);
		return str;
	}
};

bool operator==(const AssetRef& a, const AssetRef& b);
bool operator!=(const AssetRef& a, const AssetRef& b);
bool operator<(const AssetRef& a, const AssetRef& b);

uint qHash(const AssetRef &key);

// Global access
class ProjectModel;
ProjectModel* PM();

// ProjectModel stores the static data of a project.
// Access global instance with PM()
class ProjectModel
{
public:
    explicit ProjectModel();
    ~ProjectModel();
    static ProjectModel* Instance();

    AssetRef createAssetRef(AssetType type = AssetType::None);

    // Access assets
    Asset* getAsset(const AssetRef& ref);
    bool hasAsset(const AssetRef& ref);

    Part* getPart(const AssetRef& ref);
    bool hasPart(const AssetRef& ref);

    Composite* getComposite(const AssetRef& ref);
    bool hasComposite(const AssetRef& ref);

    Folder* getFolder(const AssetRef& ref);
    bool hasFolder(const AssetRef& ref);

     // NB: Returns the first part with this name
    Part* findPartByName(const QString& name);
    Composite* findCompositeByName(const QString& name);
    Folder* findFolderByName(const QString& name);

    // Direct access (be careful!)
    QMap<AssetRef, QSharedPointer<Part>> parts;
    QMap<AssetRef, QSharedPointer<Composite>> composites;
    QMap<AssetRef, QSharedPointer<Folder>> folders;

    // Change the project model
    void clear();	
    bool load(const QString& fileName, QString& reason);
    bool save(const QString& fileName);

public:
	QString fileName {};
	QList<QString> importLog;
	
private:
	int mNextId = 1;

protected:
    void JsonToFolder(const QJsonObject& obj, Folder* folder);
    void FolderToJson(const QString& name, const Folder& folder, QJsonObject* obj);
    void JsonToPart(const QJsonObject& obj, const QMap<QString, QSharedPointer<QImage>>& imageMap, Part* part);
    void PartToJson(const QString& name, const Part& part, QJsonObject* obj, QMap<QString,QSharedPointer<QImage>>* imageMap);
    void CompositeToJson(const QString& name, const Composite& comp, QJsonObject* obj);
    void JsonToComposite(const QJsonObject& obj, Composite* comp);
	QString importAndFormatProperties(const QString& assetName, const QString& properties);
};

struct Asset {
	AssetRef ref    {};
	QString name    {};
	AssetRef parent {}; // isNull if none
};

struct Folder: public Asset {
    QList<AssetRef> children; // NB: not used properly yet
};

struct Part: public Asset {
    struct Mode {
		int width;
		int height;
        int numFrames;
        int numPivots;
        int framesPerSecond;

		// Each of these are numFrames long
		QList<QSharedPointer<QImage>> frames;
        QList<QPoint> anchor;
        QList<QPoint> pivots[MAX_PIVOTS];

		// Derived and cached properties
		QRect bounds {};
    };

    QMap<QString,Mode> modes; // fourcc->mode
    QString properties;
};

struct Composite: public Asset {
    struct Child {
		AssetRef part {};
		int id = -1; // unused for now
        int index = -1; // in children
        int parent = -1; // index of parent
        int parentPivot = -1; // pivot index connected to
        int z = 0; // z-order (comp is drawn from lowest z to highest z)
        QList<int> children; // list of children (indices)
    };

    int root = -1;
    QMap<QString,Child> childrenMap;
    QList<QString> children;
	QString properties{};
};


#endif // PROJECTMODEL_H
