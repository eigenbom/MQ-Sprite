#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <QList>
#include <QImage>
#include <QMap>
#include <QString>
#include <QPoint>
#include <QJsonObject>
#include <QUuid>
#include <QSharedPointer>

static const int MAX_PIVOTS = 4;
static const int MAX_CHILDREN = 4;

struct Asset;
struct Part;
struct Composite;
struct Folder;

enum class AssetType {
    Part = 0,
    Composite = 1,
    Folder = 2
};

// TODO: Convert my v1 save files
#define PROJECT_SAVE_FILE_VERSION 2

struct AssetRef {
    QUuid uuid;
    AssetType type;
    AssetRef();
    bool isNull() const {return uuid.isNull();}
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

    AssetRef createAssetRef();

    // globally accessible assets of the model
    QString fileName;

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
    bool load(const QString& fileName);
    bool save(const QString& fileName);

protected:
    static void JsonToFolder(const QJsonObject& obj, Folder* folder);
    static void FolderToJson(const QString& name, const Folder& folder, QJsonObject* obj);
    static void JsonToPart(const QJsonObject& obj, const QMap<QString,QImage*>& imageMap, Part* part);
    static void PartToJson(const QString& name, const Part& part, QJsonObject* obj, QMap<QString,QImage*>* imageMap);
    static void CompositeToJson(const QString& name, const Composite& comp, QJsonObject* obj);
    static void JsonToComposite(const QJsonObject& obj, Composite* comp);
};

struct Asset {
    AssetRef ref;
    QString name;

    AssetRef parent; // isNull if none
};

struct Folder: public Asset {
    QList<AssetRef> children; // NB: not used properly yet
};

struct Part: public Asset {
    struct Layer {
        bool visible;
        QString name;
        QList<QSharedPointer<QImage>> frames;
    };

    struct Mode {
        int width, height;
        int numLayers;
        int numFrames;
        int numPivots;
        int framesPerSecond;

        QList<QSharedPointer<Layer>> layers; // ordered

        // Extra per-frame information
        QList<QPoint> anchor;
        QList<QPoint> pivots[MAX_PIVOTS];
    };

    QMap<QString,Mode> modes; // fourcc->mode
    QString properties;
};

struct Composite: public Asset {
    struct Child {
        AssetRef part;
        int index; // in children
        int parent; // index of parent
        int parentPivot; // pivot index connected to
        int z; // z-order (comp is drawn from lowest z to highest z)
        QList<int> children; // list of children (indices)
    };

    int root;
    QMap<QString,Child> childrenMap;
    QList<QString> children;
    QString properties;
};


#endif // PROJECTMODEL_H
