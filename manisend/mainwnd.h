#ifndef MAINWND_H
#define MAINWND_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QItemSelection>
#include <QTimer>
#include <QDir>
#include "moddir.h"
#include "modspecsumm.h"
#include "modflyers.h"
#include "modflyers.h"
#include "infownd.h"

class QNetworkReply;
class QNetworkAccessManager;
class QLabel;
class QAction;

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
    void trayActivated(QSystemTrayIcon::ActivationReason r);
    void trayMainToggle(bool checked);
    void initFLoadFiles();
    void initSpecSumm();
    void initFlyers();
    void initStatusBar();
    bool event(QEvent *pEvent);
    void refreshDir();
    void fileSelect(const QString fullname, const QString fname);
    void chkSelFile();
    void popupMessage(const QString &txt, bool isErr = false);
    void sendError(const QString &txt);
    bool sendSelFile();
    void sendDone(QNetworkReply *reply);
    void replyOpt(const QString &str);

    InfoWnd info;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QAction *actMain;
    QSettings sett;
    ModDir *dirs;
    QString selFile;
    QDateTime dtSelFileModif;
    QDateTime dtSelFileSended;
    ModSpecSumm *specsumm;
    ModFlyers *flyers;

    QTimer *tmrSendSelFile;
    QTimer *tmrReSendOnFail;

    QLabel* labSelFile;
    QLabel* labState;

    QNetworkAccessManager *httpManager;
};
#endif // MAINWND_H
