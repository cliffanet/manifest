#ifndef MODFINFO_H
#define MODFINFO_H

#include <QAbstractTableModel>
#include <QString>
#include <QList>
#include <QDateTime>

class QJsonArray;

typedef struct c_finfo_item {
    QString sheetname;
    QString flyname;
    uint    perscnt;
    char    state;
    int     before;
    int     closed_recently;
    QString modestr;
    bool    flash;
} CFInfoItem;

typedef QList<CFInfoItem> CFInfoList;

class ModFInfo: public QAbstractTableModel
{
    Q_OBJECT

public:
    ModFInfo(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void clear();
    void parseJson(const QJsonArray *_list);
    void updateModeStr();

private:
    CFInfoList list;  //holds text entered into QTableView
    QDateTime tmParse;
    bool flash;
};

#endif // MODFINFO_H
