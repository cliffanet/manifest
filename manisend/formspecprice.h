#ifndef FORMSPECPRICE_H
#define FORMSPECPRICE_H

#include <QDialog>

namespace Ui {
class FormSpecPrice;
}

class FormSpecPrice : public QDialog
{
    Q_OBJECT

public:
    explicit FormSpecPrice(QWidget *parent = nullptr);
    ~FormSpecPrice();

    void edit(const QString &_code);

    static QString settByCode(const QString &_code);
    static QString settNew();

private slots:
    void on_btnOk_clicked();
    void on_btnCancel_clicked();

private:
    Ui::FormSpecPrice *ui;
    QString code;
};

#endif // FORMSPECPRICE_H
