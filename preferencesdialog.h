#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

namespace Ui {
    class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

    QString getFijiExePath();
    void setFijiExePath( const QString &str );

    unsigned getMaxVoxelsForSV() const;
    void setMaxVoxelsForSV( unsigned val );

public slots:
    void browseFijiPathClicked();
    void spinMaxVoxValueChanged(int);

private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
