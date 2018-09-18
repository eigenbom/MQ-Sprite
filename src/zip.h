#ifndef MMPIXEL_ZIP_H
#define MMPIXEL_ZIP_H

#include <QByteArray>
#include <QMap>
#include <QString>

QMap<QString, QByteArray> LoadZip(QString filename);
QMap<QString, QString> LoadZipToFiles(QString filename);
bool WriteZip(QString filename, const QMap<QString, QString>& filenames);

// void SaveProject(ProjectModel* pm, std::string filename);
// void LoadProject(ProjectModel* pm, std::string filename);

#endif