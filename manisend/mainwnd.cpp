#include "mainwnd.h"
#include "ui_mainwnd.h"

#include <QCloseEvent>
#include <QAction>
#include <QMenu>
#include <QFileDialog>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>

#include <QSettings>
extern QSettings *sett;

MainWnd::MainWnd(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWnd)
{
    ui->setupUi(this);

    this->setWindowTitle(tr("Манифест"));

    createTrayIcon();
    trayIcon->show();

    initFLoadFiles();
    initSpecSumm();
    initFlyers();
    initStatusBar();

    on_btnFLoadRefresh_clicked();
}

MainWnd::~MainWnd()
{
    delete ui;
}

void MainWnd::appCommitData()
{
    sett->setValue("mainshow", isVisible());
    sett->setValue("infoshow", info.isVisible());
    sett->sync();
}

// Восстановление окон
void MainWnd::restoreWnd()
{
    QVariant s;
    s = sett->value("mainshow");
    if (s.isValid()) {
        if (s.toBool())
            show();
    }
    else
    if (!fcur->isAllowed())
        show();

    s = sett->value("infoshow");
    if (s.isValid()) {
        if (s.toBool())
            info.show();
    }
    else
        info.show();
}

// Кнопка "отправить текущий файл"
void MainWnd::on_btnFLoadSend_clicked()
{
    fcur->send();
}

// кнопка "Обновить директорию"
void MainWnd::on_btnFLoadRefresh_clicked()
{
    // Подготавливаемся к чтению папки
    dirs->clear();
    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(false);

    // прерываем автозагрузку файла
    fcur->clear();

    // Проверяем валидность проверяемой папки
    QVariant vdir = sett->value("dir");
    if (!vdir.isValid()) {
        // Выбранная папка рядом с кнопкой
        ui->labFLoadDir->setText("[не выбрана]");
        // Имя файла в statusbar
        labSelFile->setText("Не выбрана папка с файлами");
        return;
    }
    QString sdir = vdir.toString();

    QDir dir(sdir);
    if (!dir.exists()) {
        // Выбранная папка рядом с кнопкой
        ui->labFLoadDir->setText("[не существует] " + sdir);
        // Имя файла в statusbar
        labSelFile->setText("Выбранной папки не существует");
        return;
    }
    ui->labFLoadDir->setText(sdir);

    // парсим выбранную диру
    if (!dirs->start(sdir)) {
        // Имя файла в statusbar
        labSelFile->setText("Не найден текущий файл");
    }
}

// Выбор папки в файлами
void MainWnd::on_btnFLoadDir_clicked()
{
    // Есть ли у нас уже выбранный вариант, чтобы указать QFileDialog, какую папку открыть
    QVariant vdir = sett->value("dir");
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
    sett->setValue("dir", dir);
    sett->sync();
    on_btnFLoadRefresh_clicked();
}

// двойной клик на списке файлов
void MainWnd::on_twFLoadFiles_doubleClicked(const QModelIndex &index)
{
    // По двойному клику принудительно выберем файл для отправки
    dirs->selectForce(index.row());
}

// Изменение галки "не сохранять"
void MainWnd::on_chkNoSave_toggled(bool checked)
{
    if (checked)
        fcur->setFlag(FileLoader::NoSave);
    else
        fcur->delFlag(FileLoader::NoSave);
}

// Инициализация иконка в Tray
void MainWnd::createTrayIcon()
{
    trayIconMenu = new QMenu(this);

    actMain = new QAction(tr("Главное окно"), this);
    actMain->setCheckable(true);
    connect(actMain, &QAction::toggled, this, &MainWnd::trayMainToggle);
    trayIconMenu->addAction(actMain);

    info.actInfo = new QAction(tr("Информация"), this);
    info.actInfo->setCheckable(true);
    connect(info.actInfo, &QAction::toggled, &info, &InfoWnd::trayInfoToggle);
    trayIconMenu->addAction(info.actInfo);

    trayIconMenu->addSeparator();

    QAction *act = new QAction(tr("Выход"), this);
    connect(act, &QAction::triggered, qApp, &QCoreApplication::quit);
    trayIconMenu->addAction(act);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);

    connect(trayIcon, &QSystemTrayIcon::activated,this, &MainWnd::trayActivated);

    trayIcon->setIcon(QIcon(":/icon/icon.ico"));
}

void MainWnd::trayActivated(QSystemTrayIcon::ActivationReason r)
{
    if (r == QSystemTrayIcon::DoubleClick) {
        if (!this->isVisible()) {
            this->show();
        }
        else {
            this->hide();
        }
    }
}

void MainWnd::trayMainToggle(bool checked)
{
    if (checked) {
        this->showNormal();
        this->raise();
    }
    else {
        this->close();
    }
}

// инициализация отображения списка файлов
void MainWnd::initFLoadFiles()
{
    // выбранная директория с файлами
    dirs = new ModDir(this);
    ui->twFLoadFiles->setModel(dirs);
    connect(dirs, &ModDir::selected, this, &MainWnd::fileSelect);

    // Выбранный файл
    fcur = new FileLoader(this);
    QVariant vurl = sett->value("url");
    if (vurl.isValid())
        fcur->setUrl(vurl.toString());
    connect(fcur, &FileLoader::sendBegin,       this, &MainWnd::sendBegin);
    connect(fcur, &FileLoader::sendFinishing,   this, &MainWnd::sendFinishing);
    connect(fcur, &FileLoader::sendError,       this, &MainWnd::sendError);
    connect(fcur, &FileLoader::sendOk,          this, &MainWnd::sendOk);
    connect(fcur, &FileLoader::replyOpt,        this, &MainWnd::replyOpt);

    // Таблица с листингом файлов в выбранной директории
    ui->twFLoadFiles->setColumnWidth(0,70);
    ui->twFLoadFiles->setColumnWidth(1,120);
    //ui->twFLoadFiles->setColumnWidth(2,250);

    ui->twFLoadFiles->sortByColumn(1, Qt::DescendingOrder);
    ui->twFLoadFiles->setSortingEnabled(true);
    //ui->twFLoadFiles->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    QFont font = ui->twFLoadFiles->horizontalHeader()->font();
    font.setBold(true);
    ui->twFLoadFiles->horizontalHeader()->setFont( font );
    ui->twFLoadFiles->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter | (Qt::Alignment)Qt::TextWordWrap);
    //ui->twFLoadFiles->horizontalHeader()->setFixedHeight(ui->twFLoadFiles->horizontalHeader()->height()*2);
}

void MainWnd::initSpecSumm()
{
    specsumm = new ModSpecSumm(this);
    ui->twSpecSumm->setModel(specsumm);
    ui->twSpecSumm->setColumnWidth(0,160);
    ui->twSpecSumm->setColumnWidth(1,70);
    ui->twSpecSumm->setColumnWidth(2,70);
    ui->twSpecSumm->setColumnWidth(3,70);

    ui->twSpecSumm->sortByColumn(0, Qt::AscendingOrder);
    ui->twSpecSumm->setSortingEnabled(true);
    //ui->twSpecSumm->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    QFont font = ui->twSpecSumm->horizontalHeader()->font();
    font.setBold(true);
    ui->twSpecSumm->horizontalHeader()->setFont( font );
    ui->twSpecSumm->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter | (Qt::Alignment)Qt::TextWordWrap);
}

void MainWnd::initFlyers()
{
    flyers = new ModFlyers(this);
    ui->twFlyers->setModel(flyers);
    ui->twFlyers->setColumnWidth(0,160);
    ui->twFlyers->setColumnWidth(1,70);
    ui->twFlyers->setColumnWidth(2,70);
    ui->twFlyers->setColumnWidth(3,70);

    ui->twFlyers->sortByColumn(0, Qt::AscendingOrder);
    ui->twFlyers->setSortingEnabled(true);
    //ui->twFlyers->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    QFont font = ui->twFlyers->horizontalHeader()->font();
    font.setBold(true);
    ui->twFlyers->horizontalHeader()->setFont( font );
    ui->twFlyers->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter | (Qt::Alignment)Qt::TextWordWrap);
}

// инициализация statusbar
void MainWnd::initStatusBar()
{
    labSelFile = new QLabel(this);
    labState   = new QLabel(this);
    labSelFile->setText("Файл не выбран");
    ui->statusbar->addPermanentWidget(labSelFile);
    ui->statusbar->addPermanentWidget(labState, 1);
}

bool MainWnd::event(QEvent *pEvent)
{
    if (pEvent->type() == QEvent::Show) {
        QVariant s;
        s = sett->value("maingeom");
        if (s.isValid())
            restoreGeometry(s.toByteArray());
        s = sett->value("dircol");
        if (s.isValid())
            ui->twFLoadFiles->horizontalHeader()->restoreState(s.toByteArray());
        s = sett->value("specsummcol");
        if (s.isValid())
            ui->twSpecSumm->horizontalHeader()->restoreState(s.toByteArray());
        s = sett->value("flyerscol");
        if (s.isValid())
            ui->twFlyers->horizontalHeader()->restoreState(s.toByteArray());
        actMain->setChecked(true);
    }
    else
    if (pEvent->type() == QEvent::Hide) {
        sett->setValue("maingeom", saveGeometry());
        sett->setValue("dircol",        ui->twFLoadFiles->horizontalHeader()->saveState());
        sett->setValue("specsummcol",   ui->twSpecSumm->horizontalHeader()->saveState());
        sett->setValue("flyerscol",     ui->twFlyers->horizontalHeader()->saveState());
        sett->sync();
        actMain->setChecked(false);
    }

    return QWidget::event(pEvent);
}

// авто или ручной выбор файла в листинге директории
void MainWnd::fileSelect(const QString fullname, const QString fname)
{
    // индикация выбранного файла в statusbar
    labSelFile->setText(fname);

    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(true);

    // автозагрузчик файла
    fcur->run(fullname);
}

void MainWnd::popupMessage(const QString &txt, bool isErr)
{
    QIcon icon;
    if (isErr)
        icon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
    trayIcon->showMessage("Манифест", txt, icon, 3000);
}

void MainWnd::sendBegin(const QString &url)
{
    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(false);

    // статус отправки в statusbar
    labState->setText("Отправка файла по адресу: " + url);
}

void MainWnd::sendFinishing()
{
    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(fcur->isAllowed());

    // доп опции, которые мы запрашивали
    specsumm->clear();
    flyers->clear();
    info.finfo->clear();
}

// вывод ошибки при отправке файла
void MainWnd::sendError(const QString &msg)
{
    // статус отправки в statusbar
    labState->setText(
        "["+QDateTime::currentDateTime().toString("hh:ss")+"] " +
        "Ошибка отправки: " + msg
    );

    popupMessage("ОШИБКА: " + msg, true);
}

void MainWnd::sendOk()
{
    // статус отправки в statusbar
    labState->setText("Успешно отправлен в " + QDateTime::currentDateTime().toString("hh:mm"));

    // Сообщение
    popupMessage("Успешно загружено");
}

void MainWnd::replyOpt(const QString &str)
{
    int i = str.indexOf(' ');
    if (i<1)
        return;

    QString opt = str.left(i);
    QString jsonstr = str.right(str.length()-i-1);
    QJsonDocument loadDoc = QJsonDocument::fromJson(jsonstr.toUtf8());

    if (opt == "SPECSUMM") {
        QJsonArray list = loadDoc.array();
        specsumm->parseJson(&list);
    }
    else
    if (opt == "FLYERS") {
        QJsonArray list = loadDoc.array();
        flyers->parseJson(&list);
    }
    else
    if (opt == "FLYINFO") {
        QJsonArray list = loadDoc.array();
        info.finfo->parseJson(&list);
    }
}

