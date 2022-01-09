#include "moddir.h"

#include <QFont>
#include <QDir>
#include <QRegularExpression>
#include <QTimer>

ModDir::ModDir(QObject *parent)
    : QAbstractTableModel(parent)
{
    _autoFound = false;

    // Таймер для автообновления папки, если не смогли авто-найти текущий файл
    tmrRefresh = new QTimer(this);
    connect(tmrRefresh, &QTimer::timeout, this, &ModDir::refresh);
}

int ModDir::rowCount(const QModelIndex & /*parent*/) const
{
   return list.size();
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
                const auto &d = list[index.row()];
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
                const auto &d = list[index.row()];
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
    sort_col = column;
    sort_ord = order;

    switch (column) {
        case 0:
            std::sort(list.begin(), list.end(),
                [asc] (const CDirItem &d1, const CDirItem &d2) { return asc ? d2.sel > d1.sel : d1.sel > d2.sel; }
            );
            break;
        case 1:
            std::sort(list.begin(), list.end(),
                [asc] (const CDirItem &d1, const CDirItem &d2) { return asc ? d2.date > d1.date : d1.date > d2.date; }
            );
            break;
        case 2:
            std::sort(list.begin(), list.end(),
                [asc] (const CDirItem &d1, const CDirItem &d2) { return asc ? d2.fname > d1.fname : d1.fname > d2.fname; }
            );
            break;
    }

    emit layoutChanged();
}

void ModDir::clear()
{
    list.clear();
    _autoFound = false;
    curDir = "";
    selFName = "";
    stop();
    emit layoutChanged();
}

// обновление по указанной директории
bool ModDir::start(const QString &_dir)
{
    curDir = _dir;
    refresh();

    return _autoFound;
}

void ModDir::stop()
{
    // stop() только останавливает автообновление
    if (tmrRefresh->isActive())
        tmrRefresh->stop();
}

// обновление по ранее выбранной директории
void ModDir::refresh()
{
    // сбрасываем найденное ранее
    list.clear();
    _autoFound = false;
    selFName = "";

    // парсим
    parseDir("");

    if (_autoFound) {
        // Если при обновлении нашли файл, останавливаем таймер
        if (tmrRefresh->isActive())
            tmrRefresh->stop();
    }
    else {
        // Если файл не найден, повторим поиск через 10 сек
        if (!tmrRefresh->isActive())
            tmrRefresh->start(10000);
    }

    if (sort_col >= 0)
        sort(sort_col, sort_ord);
    // Обновляем отображение в таблице
    emit layoutChanged();
}

// принудительный выбор определённого файла
bool ModDir::selectForce(int _index)
{
    if ((_index < 0) || (_index >= list.count()))
        return false;

    auto &di = list[_index];
    // Если ничего не изменилось, выходим
    if (di.sel == SEL_FORCE)
        return false;

    bool changed = di.sel == SEL_NONE;

    // Сбрасываем выбор у всех файлов
    for (auto &d: list)
        d.sel = SEL_NONE;

    // ставим "принудительно"
    di.sel = SEL_FORCE;

    if (changed) {
        // Изменён выбранный файл
        emit selected(curDir + QDir::separator() + di.fname, di.fname);
    }
    
    // Останавливаем таймер повторного автопоиска
    if (tmrRefresh->isActive())
        tmrRefresh->stop();

    // Обновление отображения таблицы
    emit layoutChanged();
    return true;
}

// полное имя файла вместе с директорией поиска файлов
QString ModDir::selectedFullName()
{
    if ((curDir == "") || (selFName == ""))
        return "";
    return curDir + QDir::separator() + selFName;
}

// рекурсивный поиск по выбранной директории
void ModDir::parseDir(const QString subpath)
{
    if (curDir.isEmpty())
        return;

    QDir subdir(curDir + QDir::separator() + subpath);
    // Смотрим найденные имена файлов
    foreach (QString fname, subdir.entryList()) {
        if (fname.at(0) == '.')
            continue;

        QString fullname = curDir + QDir::separator() + subpath + fname;

        QFileInfo finf(fullname);
        if (finf.isSymLink())
            continue;
        if (finf.isDir()) {
            parseDir(subpath + fname + QDir::separator());
            continue;
        }

        QRegularExpression rx(REGEXP_XLSX);
        auto m = rx.match(fname);

        if (!m.hasMatch())
            continue;

        QStringList cap = m.capturedTexts();

        CDirItem di;
        di.fname = subpath + fname;

        di.date = QDate(cap[3].toUInt()+2000, cap[2].toUInt(), cap[1].toUInt());
        di.isNow =
            di.date.isValid() &&
            (di.date == QDate::currentDate());

        if (di.isNow && !_autoFound) {
            _autoFound = true;
            di.sel = SEL_AUTO;
            selFName = di.fname;
            emit autoFound(fullname, di.fname);
            emit selected(fullname, di.fname);
        }
        else {
            di.sel = SEL_NONE;
        }

        list.append(di);
    }
}
