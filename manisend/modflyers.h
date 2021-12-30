#ifndef MODFLYERS_H
#define MODFLYERS_H

#include <QAbstractTableModel>
#include <QString>
#include <QList>

typedef struct c_pers_item {
    QString name;
    QString code;
    int flycnt;
    int summ;
    QString fly;
} CPersItem;

typedef QList<CPersItem> CPersList;

class ModFlyers: public QAbstractTableModel
{
    Q_OBJECT

public:
    ModFlyers(CPersList &_list, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order) override;

private:
    CPersList *list;  //holds text entered into QTableView

};

#endif // MODFLYERS_H
