#ifndef MODFLYSUMM_H
#define MODFLYSUMM_H

#include <QAbstractTableModel>
#include <QString>
#include <QList>

class QJsonArray;

class ModFlySumm: public QAbstractTableModel
{
    Q_OBJECT

public:
    ModFlySumm(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order) override;

    void clear();
    void parseJson(const QJsonArray *_list);

private:
    typedef struct c_pers_item {
        QString name;
        int flycnt;
        int perscnt;
        int speccnt;
        int summ;
    } CItem;

    typedef QList<CItem> CPersList;

    CPersList list;  //holds text entered into QTableView
    int sort_col = -1;
    Qt::SortOrder sort_ord = Qt::AscendingOrder;
};

#endif // MODFLYSUMM_H
