#ifndef FILELOADER_H
#define FILELOADER_H

#include <QObject>
#include <QDateTime>

class QTimer;
class QNetworkAccessManager;
class QNetworkReply;

class FileLoader : public QObject
{
    Q_OBJECT
public:
    enum Flags {
        NoSave      = 0x01,

        OptSpecSumm = 0x0100,
        OptFlyers   = 0x0200,
        OptFlySumm  = 0x0400,
        OptFlyInfo  = 0x0800,
    };

    explicit FileLoader(QObject *parent = nullptr);
    bool run(const QString _filename);
    void clear();
    void check();
    bool send();

    void setUrl(const QString &_url);
    void setFlag(Flags _flag);
    void delFlag(Flags _flag);
    int  getFlags() const { return flags; };
    bool isAllowed() const { return !filename.isEmpty(); }

signals:
    void sendBegin(const QString &url);
    void sendFinishing();
    void sendError(const QString &msg);
    void sendOk();
    void replyOpt(const QString &str);

private:
    QString url;
    int flags;
    QString filename;
    QDateTime dtModif;
    QDateTime dtSended;
    QTimer *tmrCheck;
    QTimer *tmrOnFail;
    QNetworkAccessManager *http;

    void _sendErr(const QString &msg);
    void sendFinish(QNetworkReply *reply);
};

#endif // FILELOADER_H
