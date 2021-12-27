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

    initFLoadFiles();

    updateLabDir();
    refreshDir();
}

MainWnd::~MainWnd()
{
    delete ui;
}

void MainWnd::on_btnFLoadRefresh_clicked()
{
    refreshDir();
}

// Выбор папки в файлами
void MainWnd::on_btnFLoadDir_clicked()
{
    // Есть ли у нас уже выбранный вариант, чтобы указать QFileDialog, какую папку открыть
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

    // Сохраняем настройки
    sett.setValue("dir", dir);
    sett.sync();
    updateLabDir();
    refreshDir();
}


void MainWnd::on_twFLoadFiles_doubleClicked(const QModelIndex &index)
{
    // По двойному клику принудительно выберем файл для отправки
    forceSelectFile(index.row());
}

// Инициализация иконка в Tray
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

// инициализация отображения списка файлов
void MainWnd::initFLoadFiles()
{
    ui->twFLoadFiles->setModel(new ModDir(dirs, this));
    ui->twFLoadFiles->setColumnWidth(0,70);
    ui->twFLoadFiles->setColumnWidth(1,120);
    //ui->twFLoadFiles->setColumnWidth(2,250);

    ui->twFLoadFiles->sortByColumn(1, Qt::DescendingOrder);
    //ui->twFLoadFiles->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    QFont font = ui->twFLoadFiles->horizontalHeader()->font();
    font.setBold(true);
    ui->twFLoadFiles->horizontalHeader()->setFont( font );
    ui->twFLoadFiles->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter | (Qt::Alignment)Qt::TextWordWrap);
    ui->twFLoadFiles->horizontalHeader()->setFixedHeight(ui->twFLoadFiles->horizontalHeader()->height()*2);

    // Таймер для автообновления папки, если не смогли авто-найти текущий файл
    tmrRefreshDir = new QTimer(this);
    connect(tmrRefreshDir, &QTimer::timeout, this, &MainWnd::refreshDir);
}

// Обновление label с выбранной папкой
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

// Перечтение текущей папки с файлами
void MainWnd::refreshDir()
{
    // Подготавливаемся к чтению папки
    dirs.clear();
    selFile = "";
    // Перед началом таймер всегда выключаем,
    // включим его при необходимости вконце
    if (tmrRefreshDir->isActive())
        tmrRefreshDir->stop();

    // Проверяем валидность проверяемой папки
    QVariant vdir = sett.value("dir");
    if (!vdir.isValid()) {
        emit ui->twFLoadFiles->model()->layoutChanged();
        return;
    }
    QString sdir = vdir.toString();

    QDir dir(sdir);
    if (!dir.exists()) {
        emit ui->twFLoadFiles->model()->layoutChanged();
        return;
    }

    // Смотрим найденные имена файлов
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
        di.sel = SEL_NONE;

        di.date = QDate(cap[3].toUInt()+2000, cap[2].toUInt(), cap[1].toUInt());
        di.isNow =
            di.date.isValid() &&
            (di.date == QDate::currentDate());

        if (di.isNow && selFile.isEmpty()) {
            di.sel = SEL_AUTO;
            selFile = sdir + QDir::separator() + fname;
        }

        dirs.append(di);
    }

    // Обновляем отображение в таблице
    ui->twFLoadFiles->setSortingEnabled(false);
    emit ui->twFLoadFiles->model()->layoutChanged();
    ui->twFLoadFiles->setSortingEnabled(true);

    // Если не смогли найти в авторежиме, то через время ещё раз попробуем
    if (selFile.isEmpty())
        tmrRefreshDir->start(5000);
}

// Принудительный выбор файла из списка
void MainWnd::forceSelectFile(int i)
{
    if ((i < 0) || (i >= dirs.count()))
        return;

    // Сбрасываем выбор у всех файлов
    for (auto &d: dirs)
        d.sel = SEL_NONE;

    auto &d = dirs[i];
    // ставим "принудительно"
    d.sel = SEL_FORCE;
    QVariant vdir = sett.value("dir");
    selFile =
        vdir.isValid() ?
            vdir.toString() + QDir::separator() + d.fname :
            "";
    // Не забываем отключить таймер автообновления рабочей папки
    if (tmrRefreshDir->isActive())
        tmrRefreshDir->stop();

    // Обновляем отображение в таблице
    emit ui->twFLoadFiles->model()->layoutChanged();
}

