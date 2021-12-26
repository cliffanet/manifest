#ifndef MODDIR_H
#define MODDIR_H

#include <QAbstractTableModel>
#include <QString>
#include <QList>
#include <QDate>

#define REGEXP_XLSX "^(\\d?\\d)[\\.\\s](\\d?\\d)[\\.\\s](\\d\\d)\\.[xX][lL][sS][xX]$"

typedef struct c_dir_item {
    int n = 0;
    QString fname;
    QDate date;
    bool isNow;
} CDirItem;

typedef QList<CDirItem> CDirList;

class ModDir: public QAbstractTableModel
{
    Q_OBJECT

public:
    ModDir(CDirList &_dirs, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order) override;

private:
    CDirList *dirs;  //holds text entered into QTableView

};

#endif // MODDIR_H
