#include "modflyers.h"

#include <QFont>
#include <QJsonArray>
#include <QJsonObject>

ModFlyers::ModFlyers(QObject *parent)
    : QAbstractTableModel(parent)
{

}

int ModFlyers::rowCount(const QModelIndex & /*parent*/) const
{
   return list.size();
}

int ModFlyers::columnCount(const QModelIndex & /*parent*/) const
{
    return 5;
}

QVariant ModFlyers::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return QString("Имя");
            case 1: return QString("Код");
            case 2: return QString("Взлётов");
            case 3: return QString("Сумма");
            case 4: return QString("Взлёты");
        }
    }
    return QVariant();
}

QVariant ModFlyers::data(const QModelIndex &index, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            {
                auto &p = list[index.row()];
                switch (index.column()) {
                    case 0: return p.name;
                    case 1: return p.code;
                    case 2: return QString::number(p.flycnt);
                    case 3: return QString::number(p.summ);
                    case 4: return p.fly;
                }
            }
            break;

        case Qt::TextAlignmentRole:
            switch (index.column()) {
                case 1:
                case 2:
                case 3:
                    return int(Qt::AlignRight | Qt::AlignVCenter);
            }
    }

    return QVariant();
}

Qt::ItemFlags ModFlyers::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) & ~(Qt::ItemIsEditable);
}

void ModFlyers::sort(int column, Qt::SortOrder order)
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
        case 3: sortf(summ);    break;
        case 4: sortf(fly);     break;
    }

    emit layoutChanged();
}

void ModFlyers::clear()
{
    list.clear();
    emit layoutChanged();
}

void ModFlyers::parseJson(const QJsonArray *_list)
{
    list.clear();
    for (const auto &item : *_list) {
        const auto row = item.toObject();
        CItem p = {
            .name   = row["name"].toString(),
            .code   = row["code"].toString(),
            .flycnt = row["flycnt"].toInt(),
            .summ   = row["summ"].toInt(),
            .fly    = row["fly"].toString(),
        };
        list.push_back(p);
    }

    if (sort_col >= 0)
        sort(sort_col, sort_ord);
    // Обновляем отображение в таблице
    emit layoutChanged();
}
