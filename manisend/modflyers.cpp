#include "ModFlyers.h"

#include <QFont>

ModFlyers::ModFlyers(CPersList &_list, QObject *parent)
    : QAbstractTableModel(parent),
      list(&_list)
{

}

int ModFlyers::rowCount(const QModelIndex & /*parent*/) const
{
   return list->size();
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
                auto &p = (*list)[index.row()];
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

    switch (column) {
        case 0:
            std::sort(list->begin(), list->end(),
                [asc] (const CPersItem &p1, const CPersItem &p2) { return asc ? p2.name > p1.name : p1.name > p2.name; }
            );
            break;
        case 1:
            std::sort(list->begin(), list->end(),
                [asc] (const CPersItem &p1, const CPersItem &p2) { return asc ? p2.code > p1.code : p1.code > p2.code; }
            );
            break;
        case 2:
            std::sort(list->begin(), list->end(),
                [asc] (const CPersItem &p1, const CPersItem &p2) { return asc ? p2.flycnt > p1.flycnt : p1.flycnt > p2.flycnt; }
            );
            break;
        case 3:
            std::sort(list->begin(), list->end(),
                [asc] (const CPersItem &p1, const CPersItem &p2) { return asc ? p2.summ > p1.summ : p1.summ > p2.summ; }
            );
            break;
        case 4:
            std::sort(list->begin(), list->end(),
                [asc] (const CPersItem &p1, const CPersItem &p2) { return asc ? p2.fly > p1.fly : p1.fly > p2.fly; }
            );
            break;
    }

    emit layoutChanged();
}
