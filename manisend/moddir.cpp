#include "ModDir.h"

#include <QFont>

ModDir::ModDir(CDirList &_dirs, QObject *parent)
    : QAbstractTableModel(parent),
      dirs(&_dirs)
{

}

int ModDir::rowCount(const QModelIndex & /*parent*/) const
{
   return dirs->size();
}

int ModDir::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}

QVariant ModDir::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return QString("Текущий");
            case 1: return QString("Авто-Дата");
            case 2: return QString("Файл");
        }
    }
    return QVariant();
}

QVariant ModDir::data(const QModelIndex &index, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            {
                auto &d = (*dirs)[index.row()];
                switch (index.column()) {
                    case 0: return
                                d.sel == SEL_AUTO ?
                                    QString("авто") :
                                d.sel == SEL_FORCE ?
                                    QString("вручную") :
                                    QVariant();
                    case 1: return d.date.isValid() ? d.date.toString("d MMM yyyy") : QVariant();
                    case 2: return d.fname;
                }
            }
            break;

        case Qt::FontRole:
            {
                auto &d = (*dirs)[index.row()];
                if (d.sel > SEL_NONE) {
                    QFont font;
                    font.setBold(true);
                    return font;
                }
            }
            break;

        case Qt::TextAlignmentRole:
            switch (index.column()) {
                case 0:
                case 1:
                    return Qt::AlignCenter;
            }
    }

    return QVariant();
}

Qt::ItemFlags ModDir::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) & ~(Qt::ItemIsEditable);
}

void ModDir::sort(int column, Qt::SortOrder order)
{
    bool asc = order == Qt::AscendingOrder;

    switch (column) {
        case 0:
            std::sort(dirs->begin(), dirs->end(),
                [asc] (const CDirItem &d1, const CDirItem &d2) { return asc ? d2.sel > d1.sel : d1.sel > d2.sel; }
            );
            break;
        case 1:
            std::sort(dirs->begin(), dirs->end(),
                [asc] (const CDirItem &d1, const CDirItem &d2) { return asc ? d2.date > d1.date : d1.date > d2.date; }
            );
            break;
        case 2:
            std::sort(dirs->begin(), dirs->end(),
                [asc] (const CDirItem &d1, const CDirItem &d2) { return asc ? d2.fname > d1.fname : d1.fname > d2.fname; }
            );
            break;
    }

    emit layoutChanged();
}
