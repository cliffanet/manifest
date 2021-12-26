#include "mainwnd.h"
#include "ui_mainwnd.h"

#include <QCloseEvent>
#include <QAction>
#include <QMenu>
#include <QFileDialog>
#include <QStandardPaths>

MainWnd::MainWnd(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWnd)
{
    ui->setupUi(this);

    this->setWindowTitle(tr("Манифест - отслеживание взлётов"));

    createTrayIcon();
    trayIcon->show();

    updateLabDir();
    refreshDir();

    // Закладка People
    ui->twFLoadFiles->setModel(new ModDir(dirs, this));
    ui->twFLoadFiles->setColumnWidth(0,250);

    ui->twFLoadFiles->setSortingEnabled(true);
    //ui->twFLoadFiles->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    /*
    connect(
         ui->twPeople->selectionModel(),
         SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
         SLOT(peopleSelect(const QItemSelection &, const QItemSelection &))
    );
    */
    //connect(ui->twPeople, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(peopleEdit()));
    QFont font = ui->twFLoadFiles->horizontalHeader()->font();
    font.setBold(true);
    ui->twFLoadFiles->horizontalHeader()->setFont( font );
    ui->twFLoadFiles->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter | (Qt::Alignment)Qt::TextWordWrap);
    ui->twFLoadFiles->horizontalHeader()->setFixedHeight(ui->twFLoadFiles->horizontalHeader()->height()*2);
}

MainWnd::~MainWnd()
{
    delete ui;
}

void MainWnd::on_btnFLoadRefresh_clicked()
{
    refreshDir();
}

void MainWnd::on_btnFLoadDir_clicked()
{
    QVariant vdir = sett.value("dir");
    QString dir = vdir.isValid() ? vdir.toString() : "";
    if (dir.isEmpty() || !QDir(dir).exists())
        dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    dir =
        QFileDialog::getExistingDirectory(
            this,
            tr("Выбрать папку с файлами"),
            dir
        );
    if (dir.isEmpty() || !QDir(dir).exists()) {
        return;
    }

    sett.setValue("dir", dir);
    sett.sync();
    updateLabDir();
    refreshDir();
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
    connect(act, &QAction::triggered, this, &QMainWindow::raise);
    trayIconMenu->addAction(act);

    trayIconMenu->addSeparator();

    act = new QAction(tr("Выход"), this);
    connect(act, &QAction::triggered, qApp, &QCoreApplication::quit);
    trayIconMenu->addAction(act);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);

    trayIcon->setIcon(QIcon(":/icon/icon.ico"));
}

void MainWnd::updateLabDir()
{
    QVariant vdir = sett.value("dir");
    if (!vdir.isValid()) {
        ui->labFLoadDir->setText("[не выбрана]");
        return;
    }

    QString dir = vdir.toString();
    if (!QDir(dir).exists())
        dir = QString("[не существует] ") + dir;
    ui->labFLoadDir->setText(dir);
}

void MainWnd::refreshDir()
{
    dirs.clear();
    QVariant vdir = sett.value("dir");
    if (!vdir.isValid())
        return;
    QString sdir = vdir.toString();

    QDir dir(sdir);
    if (!dir.exists())
        return;

    int n = 0;
    foreach (QString fname, dir.entryList()) {
        QRegularExpression rx(REGEXP_XLSX);
        auto m = rx.match(fname);

        if (!m.hasMatch())
            continue;

        QStringList cap = m.capturedTexts();

        CDirItem di;
        di.n = ++n;
        di.fname = fname;

        di.date = QDate(cap[2].toUInt()+2000, cap[1].toUInt(), cap[0].toUInt());
        di.isNow =
            di.date.isValid() &&
            (di.date == QDate::currentDate());

        dirs.append(di);
    }
}

