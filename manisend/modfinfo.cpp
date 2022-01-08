#include "modfinfo.h"

#include <QFont>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

ModFInfo::ModFInfo(QObject *parent)
    : QAbstractTableModel(parent)
{
    tmParse = QDateTime::currentDateTime();
}

int ModFInfo::rowCount(const QModelIndex & /*parent*/) const
{
   return list.size();
}

int ModFInfo::columnCount(const QModelIndex & /*parent*/) const
{
    return 4;
}

QVariant ModFInfo::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return QString("ЛА");
            case 1: return QString("Взлёт");
            case 2: return QString("Уч-ков");
        }
    }
    return QVariant();
}

QVariant ModFInfo::data(const QModelIndex &index, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            {
                const auto &f = list[index.row()];
                if (f.flash && flash)
                    return QVariant();
                switch (index.column()) {
                    case 0: return f.sheetname;
                    case 1: return f.flyname;
                    case 2: return QVariant(f.perscnt);
                    case 3: return f.modestr;
                }
            }
            break;

        case Qt::FontRole:
            {
                const auto &f = list[index.row()];
                if (f.flash) {
                    QFont font;
                    font.setBold(true);
                    return font;
                }
            }
            break;

        case Qt::TextAlignmentRole:
            switch (index.column()) {
                case 2:
                    return int(Qt::AlignRight | Qt::AlignVCenter);
            }
    }

    return QVariant();
}

Qt::ItemFlags ModFInfo::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) & ~(Qt::ItemIsEditable);
}

void ModFInfo::clear()
{
    list.clear();
    emit layoutChanged();
}

void ModFInfo::parseJson(const QJsonArray *_list)
{
    list.clear();
    tmParse = QDateTime::currentDateTime();

    for (const auto &item : *_list) {
        const auto fly = item.toObject();
        CFInfoItem f;
        f.flyname           = fly["name"].toString();
        f.perscnt           = fly["perscnt"].toInt();
        f.state             = fly["state"].toString().at(0).toLatin1();
        f.before            = fly["before"].toInt(0);
        f.closed_recently   = fly["closed_recently"].toInt(0);

        const auto sheet = fly["sheet"].toObject();
        f.sheetname = sheet["name"].toString();

        list.push_back(f);
    }

    updateModeStr();
}

void ModFInfo::updateModeStr()
{
    int sec = tmParse.secsTo(QDateTime::currentDateTime());
    bool needTimer = false;
    bool needFlash = false;

    for (int i = list.size()-1; i >= 0; i--) {
        auto &f = list[i];
        f.flash = false;
        switch (f.state) {
            case 'b':
                if (f.before > sec) {
                    int bsec = f.before - sec;
                    f.flash =
                            ((bsec > 880) && (bsec <= 910)) ||
                            ((bsec > 580) && (bsec <= 610)) ||
                            ((bsec > 280) && (bsec <= 310)) ||
                            ((bsec > 0) && (bsec <= 30));

                    int bmin = bsec / 60;
                    bsec -= bmin*60;
                    f.modestr = QString::asprintf("готовность: %d:%02d", bmin, bsec);
                    needTimer = true;
                    if (f.flash) needFlash = true;
                }
                else
                if ((sec - f.before) < 300) {
                    f.modestr = "На старт!";
                    needTimer = true;
                }
                else {
                    list.removeAt(i);
                }
                break;

            case 't':
                if ((f.closed_recently + sec) < 600) {
                    int rsec = f.closed_recently + sec;
                    int rmin = rsec / 60;
                    rsec -= rmin*60;
                    f.modestr = QString::asprintf("улетел: %d:%02d", rmin, rsec);
                    needTimer = true;
                }
                else {
                    list.removeAt(i);
                }
                break;
        }
    }

    if (needTimer) {
        if (needFlash)
            flash = !flash;
        QTimer::singleShot(needFlash ? 330 : 1000, this, &ModFInfo::updateModeStr);
    }

    // Обновляем отображение в таблице
    emit layoutChanged();
}
