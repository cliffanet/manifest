#ifndef INFOWND_H
#define INFOWND_H

#include <QDialog>
#include "modfinfo.h"

class QAction;

namespace Ui {
class InfoWnd;
}

class InfoWnd : public QDialog
{
    Q_OBJECT

public:
    explicit InfoWnd(QWidget *parent = nullptr);
    ~InfoWnd();

    void trayInfoToggle(bool checked);
    bool event(QEvent *pEvent);

    QAction *actInfo;
    ModFInfo *finfo;

private:
    Ui::InfoWnd *ui;
};

#endif // INFOWND_H
