#include "mainwnd.h"
#include "ui_mainwnd.h"

#include <QCloseEvent>

MainWnd::MainWnd(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWnd)
{
    ui->setupUi(this);

    createTrayIcon();
    trayIcon->show();
}

MainWnd::~MainWnd()
{
    delete ui;
}

void MainWnd::createActions()
{
    /*
    minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(minimizeAction, &QAction::triggered, this, &QWidget::hide);

    maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    */
}

void MainWnd::createTrayIcon()
{
    QAction *act;

    trayIconMenu = new QMenu(this);

    act = new QAction(tr("Свернуть"), this);
    connect(act, &QAction::triggered, this, &QMainWindow::hide);
    trayIconMenu->addAction(act);

    act = new QAction(tr("Открыть"), this);
    connect(act, &QAction::triggered, this, &QMainWindow::showNormal);
    trayIconMenu->addAction(act);

    trayIconMenu->addSeparator();

    act = new QAction(tr("Выход"), this);
    connect(act, &QAction::triggered, qApp, &QCoreApplication::quit);
    trayIconMenu->addAction(act);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);

    trayIcon->setIcon(QIcon(":/icon/icon.ico"));
}

