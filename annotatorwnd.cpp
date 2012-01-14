#include "annotatorwnd.h"
#include "ui_annotatorwnd.h"

#include <QPixmap>
#include <QImage>
#include <QWheelEvent>
#include <QTimer>
#include <QFileDialog>
#include <QDebug>

#include "SuperVoxeler.h"

static SuperVoxeler<unsigned char> mSVoxel;

/** Supervoxel selection **/
struct
{
    bool  valid;    // if it contains valid selection information

    unsigned int             svIdx;     // supervoxel ID
    SlicMapType::value_type  pixelList; // pixels that are inside the selected supervoxel

} static mSelectedSV ;


AnnotatorWnd::AnnotatorWnd(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AnnotatorWnd)
{
    ui->setupUi(this);
    mSelectedSV.valid = false;  // no valid selection so far

    ui->centralWidget->setLayout( ui->horizontalLayout );

    //mVolumeData.load( "/data/phd/synapses/Rat/layer2_3stak_red_reg_z1.5_example_cropped.tif" );
    //mVolumeData.save("/tmp/test.tif" );

    mVolumeData.load("/data/phd/twoexamples/00147.tif");
    qDebug("Volume size: %dx%dx%d\n", mVolumeData.width(), mVolumeData.height(), mVolumeData.depth());

    // allocate label volume (per pixel)
    mVolumeLabels.reallocSizeLike( mVolumeData );
    mVolumeLabels.fill(0);

    ui->zSlider->setMinimum(0);
    ui->zSlider->setMaximum( mVolumeData.depth()-1 );
    ui->zSlider->setSingleStep(1);
    ui->zSlider->setPageStep(10);

    mCurZSlice = 0;

    qDebug("Computing SLIC...");
    ui->spinSVSeed->setValue( 20 );
    ui->spinSVCubeness->setValue( 40 );

    genSupervoxelClicked(); //simulate click


    ui->scrollArea->setWidget( ui->labelImg );
    ui->labelImg->setScrollArea( ui->scrollArea );;
    ui->labelImg->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    ui->labelImg->resize( ui->labelImg->pixmap()->size() );
    ui->labelImg->setScaledContents(true);

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

    if (!fileName.endsWith(".tif"))
        fileName += ".tif";

    qDebug() << fileName;

    std::string stdFName = fileName.toLocal8Bit().constData();

    if (!mVolumeLabels.save( stdFName ))
        statusBarMsg(QString("Error saving ") + fileName, 0 );
    else
        statusBarMsg("File saved successfully.");

}

void AnnotatorWnd::genSupervoxelClicked()
{
    mSVoxel.apply( mVolumeData, ui->spinSVSeed->value(), ui->spinSVCubeness->value() );
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

    std::vector<int> classLabelHue;
    classLabelHue.push_back( QColor(Qt::blue).hsvHue() );
    classLabelHue.push_back( QColor(Qt::green).hsvHue() );
    classLabelHue.push_back( QColor(Qt::red).hsvHue() );



    // check ground truth slice and draw it
    if (mOverlayLabelImage)
    {
        int intTransp = 255*mOverlayLabelImageTransparency;

        const LabelType *lblPtr = mVolumeLabels.sliceData( mCurZSlice );
        unsigned int *pixPtr = (unsigned int *) qimg.constBits(); // trick!

        unsigned int sz = mVolumeLabels.width() * mVolumeLabels.height();
        int maxLabel = (int) classLabelHue.size();

        for (unsigned int i=0; i < sz; i++)
        {
            if ( lblPtr[i] == 0 )
                continue;   // unlabeled, we don't care

            if (lblPtr[i] > maxLabel) {
                qDebug("Label exceed possible colors: %d", (int)lblPtr[i]);
                continue;
            }

            QColor c(pixPtr[i]);
            c.setHsv( classLabelHue[lblPtr[i]-1], intTransp, pixPtr[i] & 0xFF );

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

void AnnotatorWnd::labelImageMouseReleaseEvent(QMouseEvent * e)
{
    QPoint pt = ui->labelImg->screenToImage( e->pos() );

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

    // then mark it according to the GT
    const unsigned char label = (unsigned char) ui->comboLabel->currentIndex();
    for (int i=0; i < mSelectedSV.pixelList.size(); i++)
        mVolumeLabels.data()[ mSelectedSV.pixelList[i].index ] = label;

    //// this should go on a status bar, and disappear after a while
    statusBarMsg(QString("%1 pixels labeled with Label %2").arg(mSelectedSV.pixelList.size()).arg(label));

    mSelectedSV.valid = false;  //so that the current supervoxel won't be displayed after update
    updateImageSlice();
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

    if(invalid)
    {
        return;
    }

    updateCursorPixelInfo( x, y, mCurZSlice );

    if (e->modifiers() != Qt::ControlModifier)
        return; //then don't do anything, so the person can move the mouse away

    // find supervoxel idx
    unsigned int slicIdx = mSVoxel.pixelToVoxel() (x, y, mCurZSlice);
    //qDebug("Slic IDX: %u", slicIdx);

    // find corresponding pixels and copy them to the local structure
    mSelectedSV.pixelList = mSVoxel.voxelToPixel().at(slicIdx);

    mSelectedSV.svIdx = slicIdx;
    mSelectedSV.valid = true;

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

