#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <QFileDialog>

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    connect( ui->butBrowseFijiPath, SIGNAL(clicked()), this, SLOT(browseFijiPathClicked()) );
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

QString PreferencesDialog::getFijiExePath()
{
    return ui->lineEdit->text();
}

void PreferencesDialog::setFijiExePath( const QString &str )
{
    ui->lineEdit->setText( str );
}

void PreferencesDialog::browseFijiPathClicked()
{
    QString fName = QFileDialog::getOpenFileName( this, "Browse for Fiji executable" );

    if ( fName.isEmpty() )
        return;

    ui->lineEdit->setText(fName);
}
