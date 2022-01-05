#ifndef MODDIR_H
#define MODDIR_H

#include <QAbstractTableModel>
#include <QString>
#include <QList>
#include <QDate>

class QTimer;

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
    ModDir(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order) override;

    void clear();
    bool start(const QString &_dir);
    void stop();
    void refresh();
    bool selectForce(int _index);

    bool autoFounded() { return _autoFound; }
    const QString &selectedFName() { return selFName; }
    QString selectedFullName();

private:
    CDirList list;  //holds text entered into QTableView
    int sort_col = -1;
    Qt::SortOrder sort_ord = Qt::AscendingOrder;
    QString curDir;
    bool _autoFound;
    QString selFName;
    QTimer *tmrRefresh;

    void parseDir(const QString subpath);

Q_SIGNALS:
    void autoFound(const QString &fullname, const QString &fname);
    void selected(const QString &fullname, const QString &fname);
};

#endif // MODDIR_H
