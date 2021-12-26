#include "mainwnd.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Cliff");
    QCoreApplication::setOrganizationDomain("cliffa.net");
    QCoreApplication::setApplicationName("Manifest");

    QApplication a(argc, argv);
    //a.setQuitOnLastWindowClosed(false); // временно отключим на время разработки
    MainWnd w;
    w.show();
    return a.exec();
}
