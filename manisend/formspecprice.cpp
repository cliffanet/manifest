#include "formspecprice.h"
#include "ui_formspecprice.h"

#include <QRegularExpressionValidator>

#include <QSettings>
extern QSettings *sett;

FormSpecPrice::FormSpecPrice(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FormSpecPrice)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags &= ~ Qt::WindowContextHelpButtonHint;
    //flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    auto le = ui->leFly;
    le->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), le));
    le = ui->lePers;
    le->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), le));
}

FormSpecPrice::~FormSpecPrice()
{
    delete ui;
}

void FormSpecPrice::edit(const QString &_code)
{
    if (_code.isEmpty())
        return;

    code = _code;
    ui->labForVal->setText(_code);

    // Для сохранения данных о ставках у нас будет отдельная группа
    // ставки будут храниться в формате:
    // 01_code => код
    // 01_forfly => ставка за взлёт
    // 01_forpers => ставка за участника
    // Это сложно в плане поиска по коду, но даст гарантию,
    // что во всех ОС не будет проблем с форматом key,
    // т.к. _code может содержать любой символ
    sett->beginGroup("SpecPrice");
    QString suff = settByCode(_code);
    if (suff.isEmpty()) {
        ui->leFly->setText("");
        ui->lePers->setText("");
    }
    else {
        QVariant val;
        uint v;

        val = sett->value(suff + "forfly");
        v = val.isValid() ? val.toUInt() : 0;
        ui->leFly->setText(v > 0 ? QString::number(v) : "");
        val = sett->value(suff + "forpers");
        v = val.isValid() ? val.toUInt() : 0;
        ui->lePers->setText(v > 0 ? QString::number(v) : "");
    }
    sett->endGroup();

    exec();
}

QString FormSpecPrice::settByCode(const QString &_code)
{
    foreach (auto key, sett->allKeys()) {
        if (key.length() <= 4)
            continue;
        if (key.right(4) != "code")
            continue;
        if (sett->value(key).toString() != _code)
            continue;
        return key.left(key.length()-4);
    }

    return "";
}

QString FormSpecPrice::settNew()
{
    uint n = 0;
    while (n < 99) {
        n++;
        QString suff = QString::asprintf("%02d_", n);
        QVariant val = sett->value(suff + "code");
        if (!val.isValid())
            return suff;
    }
    return "xx_";
}

void FormSpecPrice::on_btnOk_clicked()
{
    if (!code.isEmpty()) {
        sett->beginGroup("SpecPrice");
        QString suff = settByCode(code);

        int
            fly = ui->leFly->text().toUInt(),
            pers= ui->lePers->text().toUInt();
        if ((fly>0) || (pers>0)) {
            if (suff.isEmpty())
                suff = settNew();
            sett->setValue(suff + "code", code);
            if (fly>0)
                sett->setValue(suff + "forfly", fly);
            else
                sett->remove(suff + "forfly");
            if (pers>0)
                sett->setValue(suff + "forpers", pers);
            else
                sett->remove(suff + "forpers");
        }
        else
        if (!suff.isEmpty()) {
            sett->remove(suff + "code");
            sett->remove(suff + "forfly");
            sett->remove(suff + "forpers");
        }

        sett->endGroup();
    }

    close();
    deleteLater();
}

void FormSpecPrice::on_btnCancel_clicked()
{
    close();
    deleteLater();
}

