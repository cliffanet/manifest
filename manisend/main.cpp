#include "mainwnd.h"

#include <QApplication>

#include <QSettings>
QSettings *sett;

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Cliff");
    QCoreApplication::setOrganizationDomain("cliffa.net");
    QCoreApplication::setApplicationName("Manifest");

    QSettings sett1;
    sett = &sett1;

    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    MainWnd w;
    w.restoreWnd();
    QObject::connect(&a, &QApplication::commitDataRequest, &w, &MainWnd::appCommitData);
    return a.exec();
}
