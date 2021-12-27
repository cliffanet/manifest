#ifndef MAINWND_H
#define MAINWND_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QItemSelection>
#include <QTimer>
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
    void on_twFLoadFiles_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWnd *ui;

    void createTrayIcon();
    void initFLoadFiles();
    void updateLabDir();
    void refreshDir();
    void forceSelectFile(int i);

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QSettings sett;
    CDirList dirs;
    QString selFile;

    QTimer *tmrRefreshDir;
};
#endif // MAINWND_H
