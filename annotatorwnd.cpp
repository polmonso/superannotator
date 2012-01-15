#include "annotatorwnd.h"
#include "ui_annotatorwnd.h"

#include <QPixmap>
#include <QImage>
#include <QWheelEvent>
#include <QTimer>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>

#include "SuperVoxeler.h"

static SuperVoxeler<unsigned char> mSVoxel;

/** Supervoxel selection **/
struct
{
    bool  valid;    // if it contains valid selection information

    unsigned int             svIdx;     // supervoxel ID
    SlicMapType::value_type  pixelList; // pixels that are inside the selected supervoxel

} static mSelectedSV ;

/** Region where supervoxels were computed
  * This is to avoid computing supervoxels for the whole volume, which could
  * be infeasible or too expensive
  */
struct Region3D
{
    bool valid; //if region is valid

    UIntPoint3D corner; // (x,y,z) corner
    UIntPoint3D size;   // (width,height,depth) from corner

    Region3D() {
        valid = false;
    }

    // initialization from two corners
    Region3D( const UIntPoint3D &c1, const UIntPoint3D &c2 ) {
        if ( c2.x < c1.x || c2.y < c1.y || c2.z < c1.z ) {
            qWarning("Region3D constructur: No proper corners given!");
            valid = false;
        }

        valid = true;
        corner = c1;
        size = UIntPoint3D( c2.x - c1.x + 1, c2.y - c1.y + 1, c2.z - c1.z + 1 );
    }

    Region3D( const QRect &qrect, unsigned int startZ, unsigned int depth )
    {
        valid = true;
        corner.x = qrect.x();
        corner.y = qrect.y();

        size.x = qrect.width();
        size.y = qrect.height();

        corner.z = startZ;
        size.z = depth;

        valid = true;
    }

    // use this region to crop a volume
    template<typename T>
    void useToCrop( const Matrix3D<T> &whole, Matrix3D<T> *cropped )
    {
        if (!valid)
            qFatal("Tried to crop volume with invalid region");

        whole.cropRegion( corner.x, corner.y, corner.z, size.x, size.y, size.z, cropped );
    }

    // returns if pt is in the given region and if withoutOffset != 0 => the un-offsetted value of pt
    bool inRegion( const UIntPoint3D &pt, UIntPoint3D *withoutOffset  )
    {
        if ( (pt.x < corner.x) || (pt.y < corner.y) || (pt.z < corner.z) )
            return false;

        unsigned int ox = pt.x - corner.x;
        unsigned int oy = pt.y - corner.y;
        unsigned int oz = pt.z - corner.z;

        if ( (ox >= size.x) || (oy >= size.y) || (oz >= size.z) )
            return false;

        if ( withoutOffset != 0 ) {
            withoutOffset->x = ox;
            withoutOffset->y = oy;
            withoutOffset->z = oz;
        }

        return true;
    }

    // converts pixel list form whole image to the non-offset one (local to the region)
    // assumes that all the pixels are inside the region, othewise there will be problems with negative values!
    template<typename T>
    void croppedToWholePixList( const Matrix3D<T> &wholeVolume, const PixelInfoList &cropped, PixelInfoList &whole)
    {
        whole.resize(cropped.size());
        for (unsigned int i=0; i < whole.size(); i++)
        {
            unsigned int dx, dy, dz;
            whole[i].coords.x = dx = cropped[i].coords.x + corner.x;
            whole[i].coords.y = dy = cropped[i].coords.y + corner.y;
            whole[i].coords.z = dz = cropped[i].coords.z + corner.z;

            whole[i].index = wholeVolume.coordToIdx( dx, dy, dz );
        }
    }

    inline unsigned int totalVoxels() {
        if (!valid)
            return 0;

        return size.x*size.y*size.z;
    }

};

static Region3D mSVRegion;

// maximum size for SV region in voxels, to avoid memory problems
static unsigned int mMaxSVRegionVoxels;

AnnotatorWnd::AnnotatorWnd(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AnnotatorWnd)
{
    ui->setupUi(this);

    // this should be configurable
    mMaxSVRegionVoxels = 200U*200U*200U;

    mSelectedSV.valid = false;  // no valid selection so far

    ui->centralWidget->setLayout( ui->horizontalLayout );

    //mVolumeData.load( "/data/phd/synapses/Rat/layer2_3stak_red_reg_z1.5_example_cropped.tif" );
    mVolumeData.load( "/data/phd/synapses/Rat/layer2_3stak_red_reg_z1.5_firstquarter.tif" );
    //mVolumeData.load( "/data/phd/synapses/Rat/layer2_3stak_red_reg_z1.5.tif" );
    //mVolumeData.save("/tmp/test.tif" );
    //mVolumeData.load("/data/phd/twoexamples/00147.tif");
    qDebug("Volume size: %dx%dx%d\n", mVolumeData.width(), mVolumeData.height(), mVolumeData.depth());

    // allocate label volume (per pixel)
    mVolumeLabels.reallocSizeLike( mVolumeData );
    mVolumeLabels.fill(0);

    ui->zSlider->setMinimum(0);
    ui->zSlider->setMaximum( mVolumeData.depth()-1 );
    ui->zSlider->setSingleStep(1);
    ui->zSlider->setPageStep(10);

    mCurZSlice = 0;

    ui->spinSVSeed->setValue( 20 );
    ui->spinSVCubeness->setValue( 40 );


    ui->scrollArea->setWidget( ui->labelImg );
    ui->labelImg->setScrollArea( ui->scrollArea );;
    ui->labelImg->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    updateImageSlice();
    ui->labelImg->resize( ui->labelImg->pixmap()->size() );
    ui->labelImg->setScaledContents(false);

    // events
    connect(ui->labelImg,SIGNAL(wheelEventSignal(QWheelEvent*)),this,SLOT(labelImageWheelEvent(QWheelEvent*)));

    connect(ui->labelImg,SIGNAL(mouseMoveEventSignal(QMouseEvent*)),this,SLOT(labelImageMouseMoveEvent(QMouseEvent*)));
    connect(ui->labelImg,SIGNAL(mouseReleaseEventSignal(QMouseEvent*)),this,SLOT(labelImageMouseReleaseEvent(QMouseEvent*)));

    ui->labelImg->setMouseTracking(true);


    connect(ui->zSlider,SIGNAL(sliderMoved(int)),this,SLOT(zSliderMoved(int)));
    connect(ui->zSlider,SIGNAL(valueChanged(int)),this,SLOT(zSliderMoved(int)));

    connect( ui->actionZoom_fit, SIGNAL(triggered()), ui->labelImg, SLOT(zoomFit()) );
    connect( ui->actionOverlay_labels, SIGNAL(triggered()), this, SLOT(actionLabelOverlayTriggered()) );

    connect( ui->actionSave_annotation, SIGNAL(triggered()), this, SLOT(actionSaveAnnotTriggered()) );
    connect( ui->actionLoad_annotation, SIGNAL(triggered()), this, SLOT(actionLoadAnnotTriggered()) );

    connect(ui->chkLabelOverlay,SIGNAL(stateChanged(int)),this,SLOT(chkLabelOverlayStateChanged(int)));

    connect(ui->dialLabelOverlayTransp,SIGNAL(valueChanged(int)),this,SLOT(dialOverlayTransparencyMoved(int)));

    connect(ui->butGenSV, SIGNAL(clicked()), this, SLOT(genSupervoxelClicked()));


    ui->chkLabelOverlay->setChecked(true);

    mOverlayLabelImage = ui->chkLabelOverlay->checkState() == Qt::Checked;

    fillLabelComboBox(3);

    dialOverlayTransparencyMoved( 0 );
    updateImageSlice();
}

void AnnotatorWnd::actionSaveAnnotTriggered()
{
    QString fileName = QFileDialog::getSaveFileName( this, "Save annotation", "", "*.tif" );

    if (fileName.isEmpty())
        return;

    if (!fileName.endsWith(".tif"))
        fileName += ".tif";

    qDebug() << fileName;

    std::string stdFName = fileName.toLocal8Bit().constData();

    if (!mVolumeLabels.save( stdFName ))
        statusBarMsg(QString("Error saving ") + fileName, 0 );
    else
        statusBarMsg("Annotation saved successfully.");

}

void AnnotatorWnd::actionLoadAnnotTriggered()
{
    QString fileName = QFileDialog::getOpenFileName( this, "Load annotation", "", "*.tif" );

    if (fileName.isEmpty())
        return;

    qDebug() << fileName;

    std::string stdFName = fileName.toLocal8Bit().constData();

    if (!mVolumeLabels.load( stdFName ))
        QMessageBox::critical(this, "Cannot open file", QString("%1 could not be read.").arg(fileName));


    if ( (mVolumeLabels.width() != mVolumeData.width()) || (mVolumeLabels.height() != mVolumeData.height()) || (mVolumeLabels.depth() != mVolumeData.depth()) )
    {
        QMessageBox::critical(this, "Dimensions do not match", "Annotation volume does not match original volume dimensions. Re-setting labels.");
        mVolumeLabels.reallocSizeLike( mVolumeData );
        mVolumeLabels.fill(0);
        updateImageSlice();
        return;
    }

    updateImageSlice();
    statusBarMsg("Annotation loaded successfully.");
}

void AnnotatorWnd::genSupervoxelClicked()
{
    //qDebug() << "Pos:  " << ui->labelImg->x() << " " <<  ui->labelImg->y();
    //qDebug() << "Size: " << ui->labelImg->width() << " " <<  ui->labelImg->height();

    // prepare Z range
    int zMin = mCurZSlice - ui->spinSVZ->value();
    int zMax = mCurZSlice + ui->spinSVZ->value();

    if (zMin < 0)   zMin = 0;
    if (zMax >= mVolumeData.depth())    zMax = mVolumeData.depth() - 1;

    // selected x,y region + whole z range
    mSVRegion = Region3D( ui->labelImg->getViewableRect(), zMin, zMax - zMin );

    if ( mSVRegion.totalVoxels() > mMaxSVRegionVoxels )
    {
        QMessageBox::critical( this, "Region too large", QString("The current region contains %1 supervoxels, exceeding the limit of %2.").arg(mSVRegion.totalVoxels()).arg(mMaxSVRegionVoxels) );
        mSVRegion.valid = false;
        updateImageSlice();
        return;
    }

    //qDebug("Region: %d %d %d %d", mSVRegion.corner.x, mSVRegion.corner.y,
      //      mSVRegion.size.x, mSVRegion.size.y );

    // crop data
    mSVRegion.useToCrop( mVolumeData, &mCroppedVolumeData );

    mSVoxel.apply( mCroppedVolumeData, ui->spinSVSeed->value(), ui->spinSVCubeness->value() );
    mSelectedSV.valid = false;

    updateImageSlice();

    statusBarMsg( QString("Done: %1 supervoxels generated.").arg( mSVoxel.numLabels() ), 0 );
}

void AnnotatorWnd::statusBarMsg( const QString &str, int timeout )
{
    statusBar()->showMessage( str, timeout );
}

void AnnotatorWnd::fillLabelComboBox( int numLabels )
{
    if (numLabels > mLblColorList.hueList.size()) {
        qWarning("fillLabelComboBox: numLabels exceeds hueList size");
        numLabels = mLblColorList.hueList.size();
    }

    ui->comboLabel->clear();

    // first none class
    ui->comboLabel->addItem("Unlabeled");

    for (int i=0; i < mLblColorList.hueList.size(); i++)
        ui->comboLabel->addItem( mLblColorList.iconList[i], QString("Class %1").arg(i+1) );

    ui->comboLabel->setCurrentIndex(1);
}

void AnnotatorWnd::dialOverlayTransparencyMoved( int )
{
    mOverlayLabelImageTransparency = ui->dialLabelOverlayTransp->value() * 1.0 / 100;
    updateImageSlice();
}


void AnnotatorWnd::actionLabelOverlayTriggered()
{
    ui->chkLabelOverlay->setChecked( ui->actionOverlay_labels->isChecked() );
}

void AnnotatorWnd::chkLabelOverlayStateChanged(int state)
{
    mOverlayLabelImage = ui->chkLabelOverlay->checkState() == Qt::Checked;
    updateImageSlice();
}

void AnnotatorWnd::updateCursorPixelInfo(int x, int y, int z)
{
    ui->labelPixInfo->setText(
                QString("Pixel (%1, %2, %3)").arg(x).arg(y).arg(z));
}

void AnnotatorWnd::zSliderMoved(int newPos)
{
    mCurZSlice = newPos;

    if (mCurZSlice < 0)
        mCurZSlice = 0;

    if (mCurZSlice >= mVolumeData.depth())
        mCurZSlice = mVolumeData.depth() - 1;

    updateImageSlice();
}

void AnnotatorWnd::updateImageSlice()
{
    QImage qimg;
    mVolumeData.QImageSlice( mCurZSlice, qimg );

    // check ground truth slice and draw it
    if (mOverlayLabelImage)
    {
        int intTransp = 255*mOverlayLabelImageTransparency;

        const LabelType *lblPtr = mVolumeLabels.sliceData( mCurZSlice );
        unsigned int *pixPtr = (unsigned int *) qimg.constBits(); // trick!

        unsigned int sz = mVolumeLabels.width() * mVolumeLabels.height();
        int maxLabel = (int) mLblColorList.hueList.size();

        for (unsigned int i=0; i < sz; i++)
        {
            if ( lblPtr[i] == 0 )
                continue;   // unlabeled, we don't care

            if (lblPtr[i] > maxLabel) {
                qDebug("Label exceed possible colors: %d", (int)lblPtr[i]);
                continue;
            }

            int vv = (pixPtr[i] & 0xFF);
            vv += 100; if (vv > 200) vv = 200;
            QColor c = QColor::fromHsv( mLblColorList.hueList[lblPtr[i]-1], intTransp, vv );

            pixPtr[i] = c.rgb();
        }
    }

    if (mSelectedSV.valid)  //if selection is valid, draw highlight
    {
        int selHue = 0;
        int selSat = 200;

        for (int i=0; i < mSelectedSV.pixelList.size(); i++)
        {
            unsigned int z = mSelectedSV.pixelList[i].coords.z;
            if ( mCurZSlice != z )
                continue;

            unsigned int x = mSelectedSV.pixelList[i].coords.x;
            unsigned int y = mSelectedSV.pixelList[i].coords.y;

            QColor color = QColor( qimg.pixel(x, y) );

            int hVal = color.value() + 100;
            if (hVal > 255) hVal = 255;
            color.setHsv( selHue, selSat, hVal );
            qimg.setPixel( x, y, color.rgb() );
        }
    }

    ui->labelImg->setPixmap( QPixmap::fromImage(qimg) );
}

void AnnotatorWnd::annotateSelectedSupervoxel()
{
    if (!mSelectedSV.valid)
        return;

    // then mark it according to the GT
    const unsigned char label = (unsigned char) ui->comboLabel->currentIndex();
    for (int i=0; i < mSelectedSV.pixelList.size(); i++)
        mVolumeLabels.data()[ mSelectedSV.pixelList[i].index ] = label;

    //// this should go on a status bar, and disappear after a while
    statusBarMsg(QString("%1 pixels labeled with Label %2").arg(mSelectedSV.pixelList.size()).arg(label));

    mSelectedSV.valid = false;  //so that the current supervoxel won't be displayed after update
}

void AnnotatorWnd::labelImageMouseReleaseEvent(QMouseEvent * e)
{
    QPoint pt = ui->labelImg->screenToImage( e->pos() );

    // select supervoxel for labeling?
    if ( e->button() == Qt::LeftButton )
    {
        if ( !mSelectedSV.valid )
            return; // no superpixel valid

        // try to find a pixel of the current supervoxel where the mouse is
        // otherwise we will not do anything
        bool found = false;
        for (int i=0; i < mSelectedSV.pixelList.size(); i++)
        {
            if (mSelectedSV.pixelList[i].coords.z != mCurZSlice)
                continue;

            if ( (mSelectedSV.pixelList[i].coords.x == pt.x()) && (mSelectedSV.pixelList[i].coords.y == pt.y())) {
                found = true;
                break;
            }
        }

        if (!found)
            return;

        annotateSelectedSupervoxel();
        updateImageSlice();
    }

    if ( e->button() == Qt::RightButton )  // iterate through possible labels
    {
        int incr = 1;
        if ( e->modifiers() == Qt::ShiftModifier )
            incr = -1;

        int newLbl = ui->comboLabel->currentIndex() + incr;
        if ( newLbl < 0 )
            newLbl = 0;
        if (newLbl >= ui->comboLabel->count())
            newLbl = ui->comboLabel->count() - 1;

        ui->comboLabel->setCurrentIndex( newLbl );
    }
}

void AnnotatorWnd::labelImageMouseMoveEvent(QMouseEvent * e)
{
    //qDebug("Mouse move: %d %d", e->x(), e->y());

    QPoint pt = ui->labelImg->screenToImage( e->pos() );


    int x = pt.x();
    int y = pt.y();



    bool invalid = false;
    if (x < 0)  invalid = true;
    if (y < 0)  invalid = true;
    if (x >= mVolumeData.width())   invalid = true;
    if (y >= mVolumeData.height())  invalid = true;

    if(invalid || (!mSVRegion.valid))
    {
        return;
    }

    updateCursorPixelInfo( x, y, mCurZSlice );

    if (e->modifiers() != Qt::ControlModifier)
        return; //then don't do anything, so the person can move the mouse away

    // convert to cropped region coordinates
    UIntPoint3D croppedCoords;
    if (!mSVRegion.inRegion( UIntPoint3D(x, y, mCurZSlice), &croppedCoords ))
        return; // nothing to do, outside cropped area

    // find supervoxel idx
    unsigned int slicIdx = mSVoxel.pixelToVoxel() (croppedCoords.x, croppedCoords.y, croppedCoords.z);
    //qDebug("Slic IDX: %u", slicIdx);

    // find corresponding pixels and copy them to the local structure
    mSVRegion.croppedToWholePixList( mVolumeData, mSVoxel.voxelToPixel().at(slicIdx), mSelectedSV.pixelList );

    // if restricted pixel values is checked..
    if ( ui->groupBoxRestrictPixLabels->isChecked() )
    {
        // make copy
        SlicMapType::value_type oldList = mSelectedSV.pixelList;

        mSelectedSV.pixelList.clear();

        for (int i=0; i < (int)oldList.size(); i++)
        {
            PixelType val = mVolumeData.data()[ oldList[i].index ];

            if ( (val < ui->spinPixMin->value()) || (val > ui->spinPixMax->value()) )
                continue;   //ignore

            mSelectedSV.pixelList.push_back( oldList[i] );
        }
    }


    mSelectedSV.svIdx = slicIdx;
    mSelectedSV.valid = true;

    // if mouse is pressed, then automatically annotate it
    if ( e->buttons() == Qt::LeftButton )
        annotateSelectedSupervoxel();

    updateImageSlice();
}

void AnnotatorWnd::labelImageWheelEvent(QWheelEvent * e)
{
    enum Operation { opNone, opZDive, opZoom, opHScroll, opVScroll };

    Operation op = opNone;

    if ( e->modifiers() == Qt::AltModifier )
        op = opZoom;
    if ( e->modifiers() == Qt::NoModifier )
        op = opZDive;
    if ( e->modifiers() == Qt::ControlModifier )
        op = opHScroll;
    if ( e->modifiers() == Qt::ShiftModifier )
        op = opVScroll;

    QPoint pt = ui->labelImg->screenToImage( e->pos() );

    switch(op)
    {
        case opZDive:
        {
            int zPos = ui->zSlider->value();

            if (e->delta() > 0)
                ui->zSlider->setValue( zPos + 1  );
            else
                ui->zSlider->setValue( zPos - 1  );

            // force handler call
            zSliderMoved( ui->zSlider->value() );

            updateCursorPixelInfo( pt.x(), pt.y(), mCurZSlice );

            break;
        }

        case opZoom:
        {
            const double zoomFactor = 1/0.9;
            double toZoom = zoomFactor;
            if (e->delta() < 0)
                toZoom = 1/toZoom;

            ui->labelImg->scaleImage( toZoom );

            break;
        }

        case opVScroll:
        case opHScroll:
        {
            const int toScroll = -e->delta()/4;

            QScrollBar *sBar = 0;
            if (op == opHScroll)
                sBar = ui->scrollArea->horizontalScrollBar();
            else
                sBar = ui->scrollArea->verticalScrollBar();

            sBar->setValue( sBar->value() + toScroll );
            break;
        }

    }

    //qDebug("Wheel signal: %d", e->delta());
}

AnnotatorWnd::~AnnotatorWnd()
{
    delete ui;
}

