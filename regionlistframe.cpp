#include "regionlistframe.h"
#include "ui_regionlistframe.h"

#include <QRect>
#include <QApplication>
#include <QDesktopWidget>

RegionListFrame::RegionListFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::RegionListFrame)
{
    ui->setupUi(this);

    connect( ui->listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
             this, SLOT(listCurrentItemChanged(QListWidgetItem*,QListWidgetItem*)));
}

void RegionListFrame::setRegionData( const QStringList &info )
{
    mRegionDescriptions = info;

    // update
    ui->listWidget->clear();
    for (int i=0; i < info.size(); i++)
    {
        QString name = QString("Region %1").arg(i+1);
        ui->listWidget->addItem( name );
    }

    ui->listWidget->setCurrentRow(0);
}

void RegionListFrame::listCurrentItemChanged( QListWidgetItem *, QListWidgetItem * )
{
    if (ui->listWidget->selectedItems().count() == 0)
        return;

    int curItem = ui->listWidget->currentRow();

    // show descr
    ui->textBrowser->setHtml( mRegionDescriptions.at(curItem) );

    emit currentRegionChanged(curItem);
}

void RegionListFrame::moveToBottomLeftCorner()
{
    const QRect scrnRect = QApplication::desktop()->rect();

    QRect wndRect;

    wndRect.setX( scrnRect.width() - width() );
    wndRect.setY( scrnRect.height() - height() );

    wndRect.setWidth( this->width() );
    wndRect.setHeight( this->height() );

    this->setGeometry( wndRect );
}

RegionListFrame::~RegionListFrame()
{
    delete ui;
}
