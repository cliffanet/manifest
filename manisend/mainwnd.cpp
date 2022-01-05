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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

MainWnd::MainWnd(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWnd)
{
    ui->setupUi(this);

    this->setWindowTitle(tr("Манифест"));

    // http-запросы
    httpManager = new QNetworkAccessManager(this);
    connect(httpManager, &QNetworkAccessManager::finished, this, &MainWnd::sendDone);

    createTrayIcon();
    trayIcon->show();

    initFLoadFiles();
    initSpecSumm();
    initFlyers();
    initStatusBar();

    refreshDir();
}

MainWnd::~MainWnd()
{
    delete ui;
}

// Кнопка "отправить текущий файл"
void MainWnd::on_btnFLoadSend_clicked()
{
    sendSelFile();
}

// кнопка "Обновить директорию"
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
    refreshDir();
}

// двойной клик на списке файлов
void MainWnd::on_twFLoadFiles_doubleClicked(const QModelIndex &index)
{
    // По двойному клику принудительно выберем файл для отправки
    dirs->selectForce(index.row());
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
        this->hide();
    }
}

// инициализация отображения списка файлов
void MainWnd::initFLoadFiles()
{
    dirs = new ModDir(this);
    ui->twFLoadFiles->setModel(dirs);

    connect(dirs, &ModDir::selected, this, &MainWnd::fileSelect);

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

    // Таймер для проверки изменений в файле для отправки на сервер
    tmrSendSelFile = new QTimer(this);
    connect(tmrSendSelFile, &QTimer::timeout, this, &MainWnd::chkSelFile);

    // Таймер для повторной отправки после неудачной
    tmrReSendOnFail = new QTimer(this);
    connect(tmrReSendOnFail, &QTimer::timeout, this, &MainWnd::sendSelFile);
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
    if (pEvent->type() == QEvent::Show)
        actMain->setChecked(true);
    else
    if (pEvent->type() == QEvent::Hide)
        actMain->setChecked(false);

    return QWidget::event(pEvent);
}

// Перечтение текущей папки с файлами
void MainWnd::refreshDir()
{
    // Подготавливаемся к чтению папки
    dirs->clear();
    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(false);

    // Проверяем валидность проверяемой папки
    QVariant vdir = sett.value("dir");
    if (!vdir.isValid()) {
        selFile = "";
        // Выбранная папка рядом с кнопкой
        ui->labFLoadDir->setText("[не выбрана]");
        // Имя файла в statusbar
        labSelFile->setText("Не выбрана папка с файлами");
        return;
    }
    QString sdir = vdir.toString();

    QDir dir(sdir);
    if (!dir.exists()) {
        selFile = "";
        // Выбранная папка рядом с кнопкой
        ui->labFLoadDir->setText("[не существует] " + sdir);
        // Имя файла в statusbar
        labSelFile->setText("Выбранной папки не существует");
        return;
    }
    ui->labFLoadDir->setText(sdir);

    // парсим выбранную диру
    if (!dirs->start(sdir)) {
        selFile = "";
        // Имя файла в statusbar
        labSelFile->setText("Не найден текущий файл");
    }
}

void MainWnd::fileSelect(const QString fullname, const QString fname)
{
    labSelFile->setText(fname);

    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(true);

    if (selFile != fullname) {
        selFile = fullname;
        sendSelFile();
    }
}

// проверка раз в сек, был ли изменён файл
void MainWnd::chkSelFile()
{
    if (selFile.isEmpty()) {
        tmrSendSelFile->stop();
        return;
    }

    if (dtSelFileSended.isValid() &&
        (dtSelFileSended.secsTo(QDateTime::currentDateTime()) >= 1800)) {
        // отправка, если мы ничего не отправляли более 30 минут,
        // т.к. если час и более не отправлять файл на сервер,
        // то данные с сервера сотрутся.
        sendSelFile();
        return;
    }

    const QFile file(selFile);
    const QFileInfo finf(file);
    if (!finf.exists())
        return;

    QDateTime modif = finf.lastModified();
    if (!modif.isValid())
        return;

    if (!dtSelFileModif.isValid() || (dtSelFileModif < modif))
        // Отправка при изменении файла
        sendSelFile();
}

void MainWnd::popupMessage(const QString &txt, bool isErr)
{
    QIcon icon;
    if (isErr)
        icon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
    trayIcon->showMessage("Манифест", txt, icon, 3000);
}

// вывод ошибки при отправке файла
void MainWnd::sendError(const QString &txt)
{
    // статус отправки в statusbar
    labState->setText(
        "["+QDateTime::currentDateTime().toString("hh:ss")+"] " +
        "Ошибка отправки: " + txt
    );

    popupMessage("ОШИБКА: " + txt, true);

    // При любой ошибке при отправке пробуюем отправить заного через 3 минуты
    if (!selFile.isEmpty())
        tmrReSendOnFail->start(180000);
}

// отправка выбранного файла
bool MainWnd::sendSelFile()
{
    // всегда останавливаем таймер перед началом отправки файла
    // запустим обратно только при завершении отправки
    if (tmrSendSelFile->isActive())
        tmrSendSelFile->stop();
    if (tmrReSendOnFail->isActive())
        tmrReSendOnFail->stop();

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

    // запоминаем lastModified файла,
    // чтобы дальше он обновлялся автоматически
    const QFileInfo finf(*file);
    dtSelFileModif = finf.lastModified();

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart partFile;
    QString fname = selFile;
    fname.remove('"');
    fname.remove('\\');
    partFile.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\""+fname+"\""));
    partFile.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-string"));
    partFile.setBodyDevice(file);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart
    multiPart->append(partFile);

    QHttpPart partOpt;
    partOpt.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"opt\""));
    partOpt.setBody("specsumm");
    multiPart->append(partOpt);
    partOpt.setBody("flyers");
    multiPart->append(partOpt);
    partOpt.setBody("flyinfo");
    multiPart->append(partOpt);

    if (ui->chkNoSave->isChecked()) {
        QHttpPart partNoSave;
        partNoSave.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"nosave\""));
        partNoSave.setBody("y");
        multiPart->append(partNoSave);
    }

    QVariant vurl = sett.value("url");
    QString surl = vurl.isValid() ? vurl.toString() : "http://monitor.my/load";
    QUrl url(surl);
    QNetworkRequest request(url);

    QNetworkReply *reply = httpManager->post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply

    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(false);

    // статус отправки в statusbar
    labState->setText("Отправка файла по адресу: " + surl);

    return true;
}

// завершение отправки файла - надо проверить возвращаемый ответ
void MainWnd::sendDone(QNetworkReply *reply)
{
    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(!selFile.isEmpty());

    // доп опции, которые мы запрашивали
    specsumm->clear();
    flyers->clear();
    info.finfo->clear();

    // Теперь отрисовываем статус операции
    QVariant vstat = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (vstat.isValid() && (vstat.toUInt() == 200)) {
        QString st = reply->readLine().trimmed();

        // успешное завершение отправки файла
        if (st == "OK") {
            // время успешной отправки
            // Оно нам пригодится, если очень долго не будет изменений
            dtSelFileSended = QDateTime::currentDateTime();

            while (!reply->atEnd())
                replyOpt(reply->readLine().trimmed());

            // Запускаем обратно таймер проверки текущего файла
            // только в случае успешной отправки,
            // а в случае любой ошибки будет повторная отправка через 3 минуты
            if (!selFile.isEmpty())
                tmrSendSelFile->start(1000);

            // статус отправки в statusbar
            labState->setText("Успешно отправлен в " + dtSelFileSended.toString("hh:mm"));

            // Сообщение
            popupMessage("Успешно загружено");
        }
        else
        if ((st.length() >= 6) && (st.left(6) == "ERROR ")) {
            sendError(st.right(st.length()-6));
        }
    }
    else {
        // status != 200
        sendError("Статус HTTP-запроса = " + (vstat.isValid() ? vstat.toString() : "-неизвестно-"));
    }

    // Удалим reply, когда управление вернётся в event-loop
    reply->deleteLater();
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
