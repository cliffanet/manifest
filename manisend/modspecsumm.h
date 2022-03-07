#ifndef MODSPECSUMM_H
#define MODSPECSUMM_H

#include <QAbstractTableModel>
#include <QString>
#include <QList>

class QJsonArray;

class ModSpecSumm: public QAbstractTableModel
{
    Q_OBJECT

public:
    typedef struct c_spec_item {
        QString name;
        QString code;
        int flycnt;
        int perscnt;
        int summ;
        QString fly;
    } CItem;

    ModSpecSumm(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order) override;

    void clear();
    void parseJson(const QJsonArray *_list);
    const CItem *byRow(int i) { return (i>=0) && (i<list.size()) ? &list[i] : nullptr; }
    void recalcCode(const QString &code);

private:
    typedef QList<CItem> CList;

    CList list;  //holds text entered into QTableView
    int sort_col = -1;
    Qt::SortOrder sort_ord = Qt::AscendingOrder;
};

#endif // MODSPECSUMM_H
