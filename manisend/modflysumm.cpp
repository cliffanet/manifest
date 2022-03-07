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
            break;

        case Qt::FontRole:
            if (list[index.row()].issumm > 0) {
                QFont font;
                font.setBold(true);
                return font;
            }
            break;
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

#define sortf(f) \
    std::sort(list.begin(), list.end(), \
        [asc] (const CItem &p1, const CItem &p2) { \
            if (p2.issumm != p1.issumm) \
                return p2.issumm > p1.issumm; \
            return asc ? p2.f > p1.f : p1.f > p2.f; \
        } \
    );

    switch (column) {
        case 0: sortf(name);    break;
        case 1: sortf(flycnt);  break;
        case 2: sortf(perscnt); break;
        case 3: sortf(speccnt); break;
        case 4: sortf(summ);    break;
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
            .issumm  = row.contains("issumm") ? row["issumm"].toInt() : 0,
        };
        list.push_back(p);
    }

    if (sort_col >= 0)
        sort(sort_col, sort_ord);
    // Обновляем отображение в таблице
    emit layoutChanged();
}
