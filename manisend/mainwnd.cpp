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
    updateLabDir();
    refreshDir();
}

// двойной клик на списке файлов
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

    // Таймер для проверки изменений в файле для отправки на сервер
    tmrSendSelFile = new QTimer(this);
    connect(tmrSendSelFile, &QTimer::timeout, this, &MainWnd::chkSelFile);
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
    QString prevFile = selFile;
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

    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(!selFile.isEmpty());

    // Если не смогли найти в авторежиме, то через время ещё раз попробуем
    if (selFile.isEmpty())
        tmrRefreshDir->start(5000);
    else
    // Загружаем выбранный файл
    if (prevFile != selFile)
        sendSelFile();
}

// Принудительный выбор файла из списка
void MainWnd::forceSelectFile(int i)
{
    if ((i < 0) || (i >= dirs.count()))
        return;

    QString prevFile = selFile;

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

    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(!selFile.isEmpty());

    // Загружаем выбранный файл
    if (!selFile.isEmpty() && (prevFile != selFile))
        sendSelFile();
}

// вывод ошибки при отправке файла
void MainWnd::sendError(QString txt)
{
    QString err = "ОШИБКА: " + txt;
    popUp.setPopupText(err);
    popUp.setPopupMode(PopUp_ERROR);
    popUp.show();

    // При любой ошибке при отправке пробуюем отправить заного через 3 минуты
     QTimer::singleShot(180000, this, &MainWnd::sendSelFile);
}

// отправка выбранного файла
bool MainWnd::sendSelFile()
{
    // всегда останавливаем таймер перед началом отправки файла
    // запустим обратно только при завершении отправки
    if (tmrSendSelFile->isActive())
        tmrSendSelFile->stop();

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

    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MainWnd::sendDone);

    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(false);

    return true;
}

// завершение отправки файла - надо проверить возвращаемый ответ
void MainWnd::sendDone(QNetworkReply *reply)
{
    // enabled для кнопки "отправка"
    ui->btnFLoadSend->setEnabled(!selFile.isEmpty());

    // Теперь отрисовываем статус операции
    QVariant vstat = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (!vstat.isValid() || (vstat.toUInt() != 200)) {
        sendError("Статус HTTP-запроса = " + (vstat.isValid() ? vstat.toString() : "-неизвестно-"));
        return;
    }
    QString st = reply->readLine();

    // успешное завершение отправки файла
    if (st == "OK") {
        // время успешной отправки
        // Оно нам пригодится, если очень долго не будет изменений
        dtSelFileSended = QDateTime::currentDateTime();

        // Запускаем обратно таймер проверки текущего файла
        // только в случае успешной отправки,
        // а в случае любой ошибки будет повторная отправка через 3 минуты
        if (!selFile.isEmpty())
            tmrSendSelFile->start(1000);

        // Сообщение
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

