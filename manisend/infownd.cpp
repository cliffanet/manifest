#include "infownd.h"
#include "ui_infownd.h"

#include <QAction>

#include <QSettings>
extern QSettings *sett;

InfoWnd::InfoWnd(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::InfoWnd)
{
    ui->setupUi(this);

    this->setWindowTitle(tr("Монитор взлётов"));
    setWindowFlags(Qt::WindowStaysOnTopHint);

    finfo = new ModFInfo(this);
    ui->twInfo->setModel(finfo);
    ui->twInfo->setColumnWidth(0,70);
    ui->twInfo->setColumnWidth(1,160);
    ui->twInfo->setColumnWidth(2,50);

    QFont font = ui->twInfo->horizontalHeader()->font();
    font.setBold(true);
    ui->twInfo->horizontalHeader()->setFont( font );
    ui->twInfo->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter | (Qt::Alignment)Qt::TextWordWrap);
}

InfoWnd::~InfoWnd()
{
    delete ui;
}

void InfoWnd::trayInfoToggle(bool checked)
{
    if (checked) {
        this->showNormal();
        this->raise();
    }
    else {
        this->hide();
    }
}

bool InfoWnd::event(QEvent *pEvent)
{
    if (pEvent->type() == QEvent::Show) {
        QVariant s;
        s = sett->value("infogeom");
        if (s.isValid())
            restoreGeometry(s.toByteArray());
        s = sett->value("infocol");
        if (s.isValid())
            ui->twInfo->horizontalHeader()->restoreState(s.toByteArray());
        actInfo->setChecked(true);
    }
    else
    if (pEvent->type() == QEvent::Hide) {
        sett->setValue("infogeom", saveGeometry());
        sett->setValue("infocol", ui->twInfo->horizontalHeader()->saveState());
        sett->sync();
        actInfo->setChecked(false);
    }

    return QMainWindow::event(pEvent);
}
