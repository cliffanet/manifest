#ifndef MODDIR_H
#define MODDIR_H

#include <QAbstractTableModel>
#include <QString>
#include <QList>
#include <QDate>

#define REGEXP_XLSX "^(\\d?\\d)[\\.\\s](\\d?\\d)[\\.\\s](\\d\\d)\\.[xX][lL][sS][xX]$"

typedef enum {
    SEL_NONE,
    SEL_AUTO,
    SEL_FORCE
} CSelMode;

typedef struct c_dir_item {
    QString fname;
    QDate date;
    bool isNow;
    CSelMode sel;
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
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order) override;

private:
    CDirList *dirs;  //holds text entered into QTableView

};

#endif // MODDIR_H
