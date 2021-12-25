#include "mainwnd.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    MainWnd w;
    w.show();
    return a.exec();
}
