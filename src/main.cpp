#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QStatusBar>
#include <QFile>
#include <QUrl>
#include <QDebug>
#include <iostream>

void myMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QString ctx = QString("%1:%2").arg(context.file).arg(context.line);

    //in this function, you can write the message to any stream!
    switch (type) {
    default: break;
    case QtDebugMsg:
        if (!msg.startsWith("nativeResourceForWindow"))
            MainWindow::Instance()->showMessage(QString("Debug: %1 %2").arg(msg,ctx), 2000);
        std::cerr << "Debug: " << msg.toStdString() << "\n";
        break;
    case QtWarningMsg:
        if (!msg.startsWith("nativeResourceForWindow"))
            MainWindow::Instance()->showMessage(QString("Warning: %1 %2").arg(msg,ctx), 2000);
        std::cerr << "Warning: " <<  msg.toStdString() << "\n";
        break;
    case QtCriticalMsg:
        if (!msg.startsWith("nativeResourceForWindow"))
            MainWindow::Instance()->showMessage(QString("Critical: %1 %2").arg(msg,ctx), 2000);
        std::cerr << "Critical: " << msg.toStdString() << "\n";
        break;
    case QtFatalMsg:
        std::cerr << "Fatal: " << msg.toStdString() << "\n";
        abort();
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);

    // QApplication::setStyle(QStyleFactory::create("fusion"));

    QCoreApplication::setOrganizationName("bp.io");
    QCoreApplication::setOrganizationDomain("bp.io");
    QCoreApplication::setApplicationName("mmpixel");

    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    
    return a.exec();
}
