#include "mainwnd.h"
#include "ui_mainwnd.h"

#include <QCloseEvent>
#include <QAction>
#include <QMenu>
#include <QFileDialog>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QHttpMultiPart>
#include <QNetworkReply>

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

void MainWnd::on_btnFLoadSend_clicked()
{
    sendSelFile();
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

void MainWnd::sendError(QString txt)
{
    QString err = "ОШИБКА: " + txt;
    popUp.setPopupText(err);
    popUp.setPopupMode(PopUp_ERROR);
    popUp.show();
}

bool MainWnd::sendSelFile()
{
    if (selFile.isEmpty()) {
        sendError("Не определён файл для отправки");
        return false;
    }

    if (!QFileInfo::exists(selFile)) {
        sendError("Выбранный файл не существует");
        return false;
    }

    QFile *file = new QFile(selFile);
    if (!file->open(QIODevice::ReadOnly)) {
        sendError("Не могу открыть файл");
        return false;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart partFile;
    partFile.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"manifest.xlsx\""));
    partFile.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-string"));
    partFile.setBodyDevice(file);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart
    multiPart->append(partFile);

    //QHttpPart partOpt;
    //partOpt.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"name\""));
    //partOpt.setBody("toto");/* toto is the name I give to my file in the server */
    //multiPart->append(partOpt);

    QVariant vurl = sett.value("url");
    QString surl = vurl.isValid() ? vurl.toString() : "http://monitor.my/load";
    QUrl url(surl);
    QNetworkRequest request(url);

    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = networkManager->post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply

    connect(networkManager, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(sendDone(QNetworkReply*)));

         //connect(reply, SIGNAL(uploadProgress(qint64, qint64)),
         //         this, SLOT  (uploadProgress(qint64, qint64)));

    return true;
}

void MainWnd::sendDone(QNetworkReply *reply)
{
    QVariant vstat = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (!vstat.isValid() || (vstat.toUInt() != 200)) {
        sendError("Статус HTTP-запроса = " + (vstat.isValid() ? vstat.toString() : "-неизвестно-"));
        return;
    }
    QString st = reply->readLine();
    if (st == "OK") {
        popUp.setPopupText("Успешно загружено");
        popUp.setPopupMode(PopUp_SUCCESS);
        popUp.show();
        return;
    }

    if ((st.length() >= 6) && (st.left(6) == "ERROR ")) {
        sendError(st.right(st.length()-6));
        return;
    }

    sendError("Неизвестная проблема при загрузке файла");
}

