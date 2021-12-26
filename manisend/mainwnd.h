#ifndef MAINWND_H
#define MAINWND_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QSettings>
#include "moddir.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWnd; }
QT_END_NAMESPACE

class MainWnd : public QMainWindow
{
    Q_OBJECT

public:
    MainWnd(QWidget *parent = nullptr);
    ~MainWnd();

private slots:
    void on_btnFLoadDir_clicked();
    void on_btnFLoadRefresh_clicked();

private:
    Ui::MainWnd *ui;

    void createTrayIcon();
    void updateLabDir();
    void refreshDir();

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QSettings sett;
    CDirList dirs;
};
#endif // MAINWND_H
