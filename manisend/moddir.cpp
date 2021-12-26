#include "ModDir.h"

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
    return 2;
}

QVariant ModDir::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return QString("Файл");
            case 1: return QString("");
        }
    }
    return QVariant();
}

QVariant ModDir::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        auto &d = (*dirs)[index.row()];
        switch (index.column()) {
            case 0: return d.fname;
            case 1: return QString("");
        }
    }

    return QVariant();
}

bool ModDir::setData(const QModelIndex &index, const QVariant &value, int role)
{
    /*
    if (role == Qt::EditRole) {
        if (!checkIndex(index))
            return false;
        //save value from editor to member m_gridData
        m_gridData[index.row()][index.column()] = value.toString();
        //for presentation purposes only: build and emit a joined string
        QString result;
        for (int row = 0; row < ROWS; row++) {
            for (int col= 0; col < COLS; col++)
                result += m_gridData[row][col] + ' ';
        }
        emit editCompleted(result);
        return true;
    }
    */
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    return false;
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
                [asc] (const CDirItem &d1, const CDirItem &d2) { return asc ? d2.fname > d1.fname : d1.fname > d2.fname; }
            );
            break;
    }

    emit layoutChanged();
}
