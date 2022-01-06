#include "fileloader.h"

#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QHttpMultiPart>
#include <QNetworkReply>

FileLoader::FileLoader(QObject *parent)
    : QObject{parent}
{
    url = "http://monitor.my/load";
    flags = OptSpecSumm | OptFlyers | OptFlyInfo;

    // http-запросы
    http = new QNetworkAccessManager(this);
    connect(http, &QNetworkAccessManager::finished, this, &FileLoader::sendFinish);

    // Таймер для проверки изменений в файле для отправки на сервер
    tmrCheck = new QTimer(this);
    connect(tmrCheck, &QTimer::timeout, this, &FileLoader::check);

    // Таймер для повторной отправки после неудачной
    tmrOnFail = new QTimer(this);
    connect(tmrOnFail, &QTimer::timeout, this, &FileLoader::send);
}

bool FileLoader::run(const QString _filename)
{
    if (_filename.isEmpty())
        return false;

    if (filename == _filename)
        return false;

    filename = _filename;

    return send();
}

void FileLoader::clear()
{
    filename = "";
    if (tmrCheck->isActive())
        tmrCheck->stop();
    if (tmrOnFail->isActive())
        tmrOnFail->stop();
}

void FileLoader::check()
{
    if (!isAllowed()) {
        tmrCheck->stop();
        return;
    }

    if (dtSended.isValid() &&
        (dtSended.secsTo(QDateTime::currentDateTime()) >= 1800)) {
        // отправка, если мы ничего не отправляли более 30 минут,
        // т.к. если час и более не отправлять файл на сервер,
        // то данные с сервера сотрутся.
        send();
        return;
    }

    const QFile file(filename);
    const QFileInfo finf(file);
    if (!finf.exists())
        return;

    QDateTime modif = finf.lastModified();
    if (!modif.isValid())
        return;

    if (!dtModif.isValid() || (dtModif < modif))
        // Отправка при изменении файла
        send();
}

bool FileLoader::send()
{
    emit sendBegin(url);

    // всегда останавливаем таймер перед началом отправки файла
    // запустим обратно только при завершении отправки
    if (tmrCheck->isActive())
        tmrCheck->stop();
    if (tmrOnFail->isActive())
        tmrOnFail->stop();

    if (filename.isEmpty()) {
        _sendErr("Не определён файл для отправки");
        return false;
    }

    if (!QFileInfo::exists(filename)) {
        _sendErr("Выбранный файл не существует");
        return false;
    }

    QFile *file = new QFile(filename);
    if (!file->open(QIODevice::ReadOnly)) {
        _sendErr("Не могу открыть файл");
        return false;
    }

    // запоминаем lastModified файла,
    // чтобы дальше он обновлялся автоматически
    const QFileInfo finf(*file);
    dtModif = finf.lastModified();

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart partFile;
    partFile.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\""+finf.fileName()+"\""));
    partFile.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-string"));
    partFile.setBodyDevice(file);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart
    multiPart->append(partFile);

    QHttpPart partOpt;
    partOpt.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"opt\""));
    if (flags & OptSpecSumm) {
        partOpt.setBody("specsumm");
        multiPart->append(partOpt);
    }
    if (flags & OptFlyers) {
        partOpt.setBody("flyers");
        multiPart->append(partOpt);
    }
    if (flags & OptFlyInfo) {
        partOpt.setBody("flyinfo");
        multiPart->append(partOpt);
    }

    if (flags & NoSave) {
        QHttpPart partNoSave;
        partNoSave.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"nosave\""));
        partNoSave.setBody("y");
        multiPart->append(partNoSave);
    }

    QNetworkRequest request(url);
    QNetworkReply *reply = http->post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply

    return true;
}

void FileLoader::setUrl(const QString &_url)
{
    url = _url;
}

void FileLoader::setFlag(Flags _flag)
{
    flags |= _flag;
}

void FileLoader::delFlag(Flags _flag)
{
    flags &= ~_flag;
}

void FileLoader::_sendErr(const QString &msg)
{
    emit sendFinishing();
    emit sendError(msg);

    if (isAllowed()) {
        // При любой ошибке при отправке пробуем отправить заного через 3 минуты
        tmrOnFail->start(180000);
        // Запускаем обратно таймер проверки текущего файла
        tmrCheck->start(1000);
    }
}

void FileLoader::sendFinish(QNetworkReply *reply)
{
    emit sendFinishing();

    // статус операции
    QVariant vstat = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (vstat.isValid() && (vstat.toUInt() == 200)) {
        QString st = reply->readLine().trimmed();

        // успешное завершение отправки файла
        if (st == "OK") {
            // время успешной отправки
            // Оно нам пригодится, если очень долго не будет изменений
            dtSended = QDateTime::currentDateTime();

            // Запускаем обратно таймер проверки текущего файла
            if (isAllowed())
                tmrCheck->start(1000);

            // Доп опции, которые мы запрашивали
            while (!reply->atEnd())
                emit replyOpt(reply->readLine().trimmed());

            emit sendOk();
        }
        else
        if ((st.length() >= 6) && (st.left(6) == "ERROR ")) {
            _sendErr(st.right(st.length()-6));
        }
        else {
            _sendErr("Некорректный ответ от сервера");
        }
    }
    else {
        // status != 200
        _sendErr("Статус HTTP-запроса = " + (vstat.isValid() ? vstat.toString() : "-неизвестно-"));
    }

    // Удалим reply, когда управление вернётся в event-loop
    reply->deleteLater();
}
