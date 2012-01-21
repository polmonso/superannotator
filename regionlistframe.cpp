#include "regionlistframe.h"
#include "ui_regionlistframe.h"

#include <QRect>
#include <QApplication>
#include <QDesktopWidget>
#include <QListWidgetItem>
#include <QFile>
#include <QFileDialog>

RegionListFrame::RegionListFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::RegionListFrame)
{
    ui->setupUi(this);

    connect( ui->listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
             this, SLOT(listCurrentItemChanged(QListWidgetItem*,QListWidgetItem*)));

    connect( ui->butSave, SIGNAL(clicked()), this, SLOT(saveAsClicked()) );
}

void RegionListFrame::setRegionData( const std::vector< ShapeStatistics<> > &info )
{
    mRegionDescriptions = info;

    // update
    ui->listWidget->clear();
    for (unsigned int i=0; i < info.size(); i++)
    {
        QString name = QString("Region %1").arg(i+1);
        QListWidgetItem *item = new QListWidgetItem( ui->listWidget );
        item->setText( name );
        item->setFlags( item->flags() | Qt::ItemIsUserCheckable );
        item->setCheckState( (mRegionDescriptions[i].annotationLabel() == 0) ? Qt::Unchecked : Qt::Checked );
        ui->listWidget->addItem( item );
    }

    ui->listWidget->setCurrentRow(0);
}

/*** Saves a txt file: "<LABEL_ID>\t<NUM_PIXS>\t<ANNOTATION_LABEL>"
  *  where ANNOTATION_LABEL is 0 or 1 according to the checkbox
  *  NUM_PIXS is saved to provide a safe key if those labels are used later on
  */
void RegionListFrame::saveAsClicked()
{
    QString fName = QFileDialog::getSaveFileName( this, "Save region annotation", ".", "*.txt" );
    if (fName.isEmpty())
        return;

    QFile file(fName);
    file.open( QIODevice::WriteOnly );

    QString strFmt = "%1\t%2\t%3\n";

    // add descr
    file.write( ("%%" + strFmt.arg("LABEL_ID").arg("NUM_PIXS").arg("ANNOTATION_LABEL")).toLatin1() );

    for (unsigned int i=0; i < mRegionDescriptions.size(); i++)
    {
        unsigned int checked = ui->listWidget->item( i )->checkState() == Qt::Checked;
        file.write( strFmt.arg( mRegionDescriptions[i].labelIdx() ).arg( mRegionDescriptions[i].numVoxels() ).arg( checked ).toLatin1() );
    }

    file.close();
}

void RegionListFrame::listCurrentItemChanged( QListWidgetItem *, QListWidgetItem * )
{
    if (ui->listWidget->selectedItems().count() == 0)
        return;

    int curItem = ui->listWidget->currentRow();

    // show descr
    QString s = QString("<b>Region Label = %1</b><br>").arg( mRegionDescriptions.at(curItem).labelIdx() );
    s += mRegionDescriptions.at(curItem).toString();
    ui->textBrowser->setHtml( s );

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
