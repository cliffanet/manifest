#ifndef MAINWND_H
#define MAINWND_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QItemSelection>
#include <QTimer>
#include "moddir.h"
#include "modspecsumm.h"
#include "modflyers.h"

class QNetworkReply;
class QLabel;
class QJsonArray;

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
    void on_btnFLoadSend_clicked();
    void on_btnFLoadRefresh_clicked();
    void on_btnFLoadDir_clicked();
    void on_twFLoadFiles_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWnd *ui;

    void createTrayIcon();
    void initFLoadFiles();
    void initSpecSumm();
    void initFlyers();
    void initStatusBar();
    void updateLabDir();
    void refreshDir();
    void forceSelectFile(int i);
    void chkSelFile();
    void popupMessage(const QString &txt, bool isErr = false);
    void sendError(const QString &txt);
    bool sendSelFile();
    void sendDone(QNetworkReply *reply);
    void replyOpt(const QString &str);
    void replySpecSumm(const QJsonArray *list);
    void replyFlyers(const QJsonArray *list);

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QSettings sett;
    CDirList dirs;
    QString selFile;
    QDateTime dtSelFileModif;
    QDateTime dtSelFileSended;
    CSpecList specsumm;
    CPersList flyers;

    QTimer *tmrRefreshDir;
    QTimer *tmrSendSelFile;

    QLabel* labSelFile;
    QLabel* labState;
};
#endif // MAINWND_H
