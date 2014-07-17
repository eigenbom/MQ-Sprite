#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <QList>
#include <QImage>
#include <QMap>
#include <QString>
#include <QPoint>
#include <QJsonObject>

static const int MAX_PIVOTS = 4;
static const int MAX_CHILDREN = 4;
class Part;
struct Composite;

#define ASSET_TYPE_PART 0
#define ASSET_TYPE_COMPOSITE 1

#define PROJECT_SAVE_FILE_VERSION 1

/**
 * ProjectModel stores the static data of a project and provides Qt Models on that data.
 * It is modified by QCommands supporting undo/redo (see http://qt-project.org/doc/qt-5.0/qtdoc/qundo.html )
 */
class ProjectModel
{
public:
    explicit ProjectModel();
    ~ProjectModel();
    static ProjectModel* Instance();

    // globally accessible assets of the model
    QMap<QString,Part*> parts;
    QMap<QString,Composite*> composites;
    QString fileName;

    // Change the project model
    void clear();
    void loadTestData();
    bool load(const QString& fileName);
    bool save(const QString& fileName);

protected:
    static void JsonToPart(const QJsonObject& obj, const QMap<QString,QImage*>& imageMap, Part* part);
    static void PartToJson(const QString& name, const Part& part, QJsonObject* obj, QMap<QString,QImage*>* imageMap);
    static void CompositeToJson(const QString& name, const Composite& comp, QJsonObject* obj);
    static void JsonToComposite(const QJsonObject& obj, Composite* comp);
};

class Part {
public:
    ~Part();

    struct Mode {
        int width, height;
        int numFrames;
        int numPivots;
        int framesPerSecond;
        // bool looping; // we know whether something is looping in the driver that runs it
        QList<QImage*> images; // test: QImage img(16,16); img.fill(QColor(255,0,0));
        QList<QPoint> anchor;
        QList<QPoint> pivots[MAX_PIVOTS];        
    };

    QMap<QString,Mode> modes; // fourcc->mode
    QString properties;
};

struct Composite {
    struct Child {
        QString part;
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
