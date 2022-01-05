#include "mainwnd.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Cliff");
    QCoreApplication::setOrganizationDomain("cliffa.net");
    QCoreApplication::setApplicationName("Manifest");

    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    MainWnd w;
    return a.exec();
}
