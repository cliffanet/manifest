#ifndef MAINWND_H
#define MAINWND_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include "moddir.h"
#include "fileloader.h"
#include "modspecsumm.h"
#include "modflyers.h"
#include "modflysumm.h"
#include "infownd.h"

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
    void appCommitData();
    void restoreWnd();

private slots:
    void on_btnFLoadSend_clicked();
    void on_btnFLoadRefresh_clicked();
    void on_btnFLoadDir_clicked();
    void on_twFLoadFiles_doubleClicked(const QModelIndex &index);
    void on_chkNoSave_toggled(bool checked);
    void on_btnSpecPrice_clicked();

private:
    Ui::MainWnd *ui;

    void createTrayIcon();
    void trayActivated(QSystemTrayIcon::ActivationReason r);
    void trayMainToggle(bool checked);
    void initFLoadFiles();
    void initSpecSumm();
    void initFlyers();
    void initFlySumm();
    void initStatusBar();
    bool event(QEvent *pEvent);
    void fileSelect(const QString fullname, const QString fname);
    void popupMessage(const QString &txt, bool isErr = false);

    void sendBegin(const QString &url);
    void sendFinishing();
    void sendError(const QString &msg);
    void sendOk();
    void replyOpt(const QString &str);

    void specSelect();

    InfoWnd info;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QAction *actMain;
    ModDir *dirs;
    FileLoader *fcur;
    ModSpecSumm *specsumm;
    ModFlyers *flyers;
    ModFlySumm *flysumm;

    QLabel* labSelFile;
    QLabel* labState;
};
#endif // MAINWND_H
