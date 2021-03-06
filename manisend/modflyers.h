#ifndef MODFLYERS_H
#define MODFLYERS_H

#include <QAbstractTableModel>
#include <QString>
#include <QList>

class QJsonArray;

class ModFlyers: public QAbstractTableModel
{
    Q_OBJECT

public:
    ModFlyers(QObject *parent = nullptr);
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
        QString code;
        int flycnt;
        int summ;
        QString fly;
    } CItem;

    typedef QList<CItem> CList;

    CList list;  //holds text entered into QTableView
    int sort_col = -1;
    Qt::SortOrder sort_ord = Qt::AscendingOrder;
};

#endif // MODFLYERS_H
