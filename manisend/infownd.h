#ifndef INFOWND_H
#define INFOWND_H

#include <QMainWindow>
#include "modfinfo.h"

class QAction;

namespace Ui {
class InfoWnd;
}

class InfoWnd : public QMainWindow
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
