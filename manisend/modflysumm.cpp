#include "modflysumm.h"

#include <QFont>
#include <QJsonArray>
#include <QJsonObject>

ModFlySumm::ModFlySumm(QObject *parent)
    : QAbstractTableModel(parent)
{

}

int ModFlySumm::rowCount(const QModelIndex & /*parent*/) const
{
   return list.size();
}

int ModFlySumm::columnCount(const QModelIndex & /*parent*/) const
{
    return 5;
}

QVariant ModFlySumm::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return QString("ЛА");
            case 1: return QString("Подъёмов");
            case 2: return QString("Участников");
            case 3: return QString("Персонал");
            case 4: return QString("Сумма");
        }
    }
    return QVariant();
}

QVariant ModFlySumm::data(const QModelIndex &index, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            {
                auto &p = list[index.row()];
                switch (index.column()) {
                    case 0: return p.name;
                    case 1: return QString::number(p.flycnt);
                    case 2: return QString::number(p.perscnt);
                    case 3: return QString::number(p.speccnt);
                    case 4: return QString::number(p.summ);
                }
            }
            break;

        case Qt::TextAlignmentRole:
            switch (index.column()) {
                case 1:
                case 2:
                case 3:
                case 4:
                    return int(Qt::AlignRight | Qt::AlignVCenter);
            }
    }

    return QVariant();
}

Qt::ItemFlags ModFlySumm::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) & ~(Qt::ItemIsEditable);
}

void ModFlySumm::sort(int column, Qt::SortOrder order)
{
    bool asc = order == Qt::AscendingOrder;
    sort_col = column;
    sort_ord = order;

    switch (column) {
        case 0:
            std::sort(list.begin(), list.end(),
                [asc] (const CItem &p1, const CItem &p2) { return asc ? p2.name > p1.name : p1.name > p2.name; }
            );
            break;
        case 1:
            std::sort(list.begin(), list.end(),
                [asc] (const CItem &p1, const CItem &p2) { return asc ? p2.flycnt > p1.flycnt : p1.flycnt > p2.flycnt; }
            );
            break;
        case 2:
            std::sort(list.begin(), list.end(),
                [asc] (const CItem &p1, const CItem &p2) { return asc ? p2.perscnt > p1.perscnt : p1.perscnt > p2.perscnt; }
            );
            break;
        case 3:
            std::sort(list.begin(), list.end(),
                [asc] (const CItem &p1, const CItem &p2) { return asc ? p2.speccnt > p1.speccnt : p1.speccnt > p2.speccnt; }
            );
            break;
        case 4:
            std::sort(list.begin(), list.end(),
                [asc] (const CItem &p1, const CItem &p2) { return asc ? p2.summ > p1.summ : p1.summ > p2.summ; }
            );
            break;
    }

    emit layoutChanged();
}

void ModFlySumm::clear()
{
    list.clear();
    emit layoutChanged();
}

void ModFlySumm::parseJson(const QJsonArray *_list)
{
    list.clear();
    for (const auto &item : *_list) {
        const auto row = item.toObject();
        CItem p = {
            .name    = row["name"].toString(),
            .flycnt  = row["flycnt"].toInt(),
            .perscnt = row["perscnt"].toInt(),
            .speccnt = row["speccnt"].toInt(),
            .summ    = row["summ"].toInt(),
        };
        list.push_back(p);
    }

    if (sort_col >= 0)
        sort(sort_col, sort_ord);
    // Обновляем отображение в таблице
    emit layoutChanged();
}
