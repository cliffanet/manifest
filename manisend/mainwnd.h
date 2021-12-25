#ifndef MAINWND_H
#define MAINWND_H

#include <QMainWindow>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWnd; }
QT_END_NAMESPACE

class MainWnd : public QMainWindow
{
    Q_OBJECT

public:
    MainWnd(QWidget *parent = nullptr);
    ~MainWnd();

private:
    Ui::MainWnd *ui;

    void createTrayIcon();

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
};
#endif // MAINWND_H
