#ifndef MMPIXEL_ZIP_H
#define MMPIXEL_ZIP_H

#include <QByteArray>
#include <QMap>
#include <QString>

QMap<QString, QByteArray> LoadZip(QString filename);
// QMap<QString, QByteArray> LoadZip(QString filename);

// void SaveProject(ProjectModel* pm, std::string filename);
// void LoadProject(ProjectModel* pm, std::string filename);

#endif