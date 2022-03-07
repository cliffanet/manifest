#include "modspecsumm.h"
#include "formspecprice.h"

#include <QFont>
#include <QJsonArray>
#include <QJsonObject>

#include <QSettings>
extern QSettings *sett;

ModSpecSumm::ModSpecSumm(QObject *parent)
    : QAbstractTableModel(parent)
{

}

int ModSpecSumm::rowCount(const QModelIndex & /*parent*/) const
{
   return list.size();
}

int ModSpecSumm::columnCount(const QModelIndex & /*parent*/) const
{
    return 6;
}

QVariant ModSpecSumm::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return QString("Имя");
            case 1: return QString("Код");
            case 2: return QString("Взлётов");
            case 3: return QString("Человек");
            case 4: return QString("Сумма");
            case 5: return QString("Взлёты");
        }
    }
    return QVariant();
}

QVariant ModSpecSumm::data(const QModelIndex &index, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            {
                auto &p = list[index.row()];
                switch (index.column()) {
                    case 0: return p.name;
                    case 1: return p.code;
                    case 2: return QString::number(p.flycnt);
                    case 3: return QString::number(p.perscnt);
                    case 4: return p.summ > 0 ? QString::number(p.summ) : QVariant();
                    case 5: return p.fly;
                }
            }
            break;

        case Qt::TextAlignmentRole:
            switch (index.column()) {
                case 2:
                case 3:
                case 4:
                    return int(Qt::AlignRight | Qt::AlignVCenter);
            }
    }

    return QVariant();
}

Qt::ItemFlags ModSpecSumm::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) & ~(Qt::ItemIsEditable);
}

void ModSpecSumm::sort(int column, Qt::SortOrder order)
{
    bool asc = order == Qt::AscendingOrder;
    sort_col = column;
    sort_ord = order;

#define sortf(f) \
    std::sort(list.begin(), list.end(), \
        [asc] (const CItem &p1, const CItem &p2) { \
            return asc ? p2.f > p1.f : p1.f > p2.f; \
        } \
    );

    switch (column) {
        case 0: sortf(name);    break;
        case 1: sortf(code);    break;
        case 2: sortf(flycnt);  break;
        case 3: sortf(perscnt); break;
        case 4: sortf(summ);    break;
        case 5: sortf(fly);     break;
    }

    emit layoutChanged();
}

void ModSpecSumm::clear()
{
    list.clear();
    emit layoutChanged();
}

void ModSpecSumm::parseJson(const QJsonArray *_list)
{
    list.clear();
    sett->beginGroup("SpecPrice");

    for (const auto &item : *_list) {
        const auto row = item.toObject();
        CItem s = {
            .name   = row["name"].toString(),
            .code   = row["code"].toString(),
            .flycnt = row["flycnt"].toInt(),
            .perscnt= row["perscnt"].toInt(),
            .summ   = 0,
            .fly    = row["fly"].toString(),
        };

        if (!s.code.isEmpty()) {
            QString suff = FormSpecPrice::settByCode(s.code);
            if (!suff.isEmpty()) {
                QVariant val;
                val = sett->value(suff + "forfly");
                if (val.isValid())
                    s.summ += val.toUInt() * s.flycnt;
                val = sett->value(suff + "forpers");
                if (val.isValid())
                    s.summ += val.toUInt() * s.perscnt;
            }
        }

        list.push_back(s);
    }

    sett->endGroup();

    if (sort_col >= 0)
        sort(sort_col, sort_ord);
    // Обновляем отображение в таблице
    emit layoutChanged();
}

void ModSpecSumm::recalcCode(const QString &code)
{
    QVariant val;
    sett->beginGroup("SpecPrice");
    QString suff = FormSpecPrice::settByCode(code);
    val = sett->value(suff + "forfly");
    uint forfly = val.isValid() ? val.toUInt() : 0;
    val = sett->value(suff + "forpers");
    uint forpers = val.isValid() ? val.toUInt() : 0;
    sett->endGroup();

    for (auto &s : list) {
        if (s.code != code)
            continue;
        s.summ = forfly*s.flycnt + forpers*s.perscnt;
    }

    emit layoutChanged();
}
