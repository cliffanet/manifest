#include "modspecsumm.h"

#include <QFont>
#include <QJsonArray>
#include <QJsonObject>

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
    return 5;
}

QVariant ModSpecSumm::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return QString("Имя");
            case 1: return QString("Код");
            case 2: return QString("Взлётов");
            case 3: return QString("Человек");
            case 4: return QString("Взлёты");
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
                    case 4: return p.fly;
                }
            }
            break;

        case Qt::TextAlignmentRole:
            switch (index.column()) {
                case 2:
                case 3:
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

    switch (column) {
        case 0:
            std::sort(list.begin(), list.end(),
                [asc] (const CSpecItem &p1, const CSpecItem &p2) { return asc ? p2.name > p1.name : p1.name > p2.name; }
            );
            break;
        case 1:
            std::sort(list.begin(), list.end(),
                [asc] (const CSpecItem &p1, const CSpecItem &p2) { return asc ? p2.code > p1.code : p1.code > p2.code; }
            );
            break;
        case 2:
            std::sort(list.begin(), list.end(),
                [asc] (const CSpecItem &p1, const CSpecItem &p2) { return asc ? p2.flycnt > p1.flycnt : p1.flycnt > p2.flycnt; }
            );
            break;
        case 3:
            std::sort(list.begin(), list.end(),
                [asc] (const CSpecItem &p1, const CSpecItem &p2) { return asc ? p2.perscnt > p1.perscnt : p1.perscnt > p2.perscnt; }
            );
            break;
        case 4:
            std::sort(list.begin(), list.end(),
                [asc] (const CSpecItem &p1, const CSpecItem &p2) { return asc ? p2.fly > p1.fly : p1.fly > p2.fly; }
            );
            break;
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
    for (const auto &item : *_list) {
        const auto row = item.toObject();
        CSpecItem s = {
            .name   = row["name"].toString(),
            .code   = row["code"].toString(),
            .flycnt = row["flycnt"].toInt(),
            .perscnt= row["perscnt"].toInt(),
            .fly    = row["fly"].toString(),
        };
        list.push_back(s);
    }

    if (sort_col >= 0)
        sort(sort_col, sort_ord);
    // Обновляем отображение в таблице
    emit layoutChanged();
}
