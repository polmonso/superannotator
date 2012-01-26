#include "annotatorwnd.h"
#include "ui_annotatorwnd.h"

#include <QPixmap>
#include <QImage>
#include <QWheelEvent>
#include <QTimer>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QLibrary>
#include "textinfodialog.h"

#include "FijiHelper.h"

#include "SuperVoxeler.h"
#include "regionlistframe.h"

#include "RegionGrowing.h"

#include "PluginBase.h"

static SuperVoxeler<unsigned char> mSVoxel;

/** Supervoxel selection **/
struct
{
    bool  valid;    // if it contains valid selection information

    unsigned int             svIdx;     // supervoxel ID
    SlicMapType::value_type  pixelList; // pixels that are inside the selected supervoxel

} static mSelectedSV ;

static Region3D mSVRegion;

// this holds a pointer to the regionListFrame window (if there is one)
// and other info
struct
{
    RegionListFrame                 *pFrame;
    Matrix3D<unsigned int>          lblMatrix;
    unsigned int                    lblCount;
    std::vector<ShapeStatistics<> >    shapeInfo;
    Region3D                        region3D;  // region used at computation time, to convert back to whole image coordinate sytem

} static mLabelListData;


// maximum size for SV region in voxels, to avoid memory problems
static unsigned int mMaxSVRegionVoxels;

static PluginServicesList  mPluginServList;

AnnotatorWnd::AnnotatorWnd(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AnnotatorWnd)
{
    ui->setupUi(this);

    mLabelListData.pFrame = 0;

    // settings
    m_sSettingsFile = QApplication::applicationDirPath() + "/settings.ini";
    qDebug() << m_sSettingsFile;
    loadSettings();

    mFileTypeFilter = "TIF (*.tif *.tiff)";


    mScoreImageEnabled = false;

    // this should be configurable
    mMaxSVRegionVoxels = 200U*200U*200U;

    mSelectedSV.valid = false;  // no valid selection so far

    ui->centralWidget->setLayout( ui->horizontalLayout );

    // ask user to open raw file
    {
        QString fileName = QFileDialog::getOpenFileName( 0, "Load image", mSettingsData.loadPathVolume, mFileTypeFilter );

        if (fileName.isEmpty()) {
            QTimer::singleShot(1, qApp, SLOT(quit()));
            return;
        }

        std::string stdFName = fileName.toLocal8Bit().constData();

        try {
            mVolumeData.load( stdFName );
        } catch (std::exception &e)
        {
            QMessageBox::critical( this, "Cannot open image file", "Could not open the specified image, quitting.." );
            QTimer::singleShot(1, qApp, SLOT(quit()));
            return;
        }

        mSettingsData.loadPathVolume = QFileInfo( fileName ).absolutePath();
        this->saveSettings();
    }


    //mVolumeData.load( "/data/phd/synapses/Rat/layer2_3stak_red_reg_z1.5_example_cropped.tif" );
    //mVolumeData.load( "/data/phd/synapses/Rat/layer2_3stak_red_reg_z1.5_firstquarter.tif" );
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

    connect( ui->actionScoreImageLoad, SIGNAL(triggered()), this, SLOT(actionLoadScoreImageTriggered()) );
    connect( ui->actionScoreImageEnabled, SIGNAL(triggered()), this, SLOT(actionEnableScoreImageTriggered()) );

    connect(ui->chkLabelOverlay,SIGNAL(stateChanged(int)),this,SLOT(chkLabelOverlayStateChanged(int)));

    connect(ui->dialLabelOverlayTransp,SIGNAL(valueChanged(int)),this,SLOT(dialOverlayTransparencyMoved(int)));

    connect(ui->butGenSV, SIGNAL(clicked()), this, SLOT(genSupervoxelClicked()));

    connect( ui->butConnectivityRun, SIGNAL(clicked()), this, SLOT(butRunConnectivityCheckNowClicked()) );

    connect(ui->butAnnotVis3D, SIGNAL(clicked()), this, SLOT(annotVis3DClicked()));


    ui->chkLabelOverlay->setChecked(true);

    mOverlayLabelImage = ui->chkLabelOverlay->checkState() == Qt::Checked;

    fillLabelComboBox(3);

    dialOverlayTransparencyMoved( 0 );
    updateImageSlice();

    scanPlugins( qApp->applicationDirPath() + "/plugins/" );

    this->showMaximized();
}


void AnnotatorWnd::scanPlugins( const QString &pluginFolder )
{
    // list files
    QDir dir(pluginFolder);
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);

    qDebug() << "Plugin folder: " << pluginFolder;

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);

        QString absFilePath = fileInfo.absoluteFilePath();
        if ( !QLibrary::isLibrary( absFilePath ) )
            continue;

        QLibrary library( absFilePath );
        if (!library.load()) {
            qDebug() << "Could not load plugin " << absFilePath << ":" << library.errorString();
            continue;
        }

        typedef PluginBase*(*PluginCreateFunction)(void);

        PluginCreateFunction createPlugin = (PluginCreateFunction) library.resolve("createPlugin");

        if (createPlugin == 0) {
            qDebug() << "Could not resolve entry point for plugin " << absFilePath;
            continue;
        }


        PluginBase *newPlugin = createPlugin();
        mPluginServList.append( PluginServices( newPlugin->pluginName(), this ) );
        bool ret = newPlugin->initializePlugin( mPluginServList.last() );

        if (!ret) {
            qDebug() << "Initialization failed for " << absFilePath;
            continue;
        }
    }

    if ( mPluginServList.isEmpty() )
        ui->menuPlugins->addAction( "No plugins loaded" )->setEnabled(false);
}

void AnnotatorWnd::loadSettings()
{
    QSettings settings(m_sSettingsFile, QSettings::NativeFormat);

    mSettingsData.savePath = settings.value("savePath", ".").toString();
    mSettingsData.loadPath = settings.value("loadPath", ".").toString();
    mSettingsData.loadPathScores = settings.value("loadPathScores", ".").toString();
    mSettingsData.loadPathVolume = settings.value("loadPathVolume", ".").toString();

    ui->spinSVCubeness->setValue( settings.value("spinSVCubeness", 40).toInt() );
    ui->spinSVSeed->setValue( settings.value("spinSVSeed", 20).toInt() );
    ui->spinSVZ->setValue( settings.value("spinSVZ", 100).toInt() );
}

void AnnotatorWnd::saveSettings()
{
    QSettings settings(m_sSettingsFile, QSettings::NativeFormat);
    settings.setValue( "savePath", mSettingsData.savePath );
    settings.setValue( "loadPath", mSettingsData.loadPath );
    settings.setValue( "loadPathScores", mSettingsData.loadPathScores );
    settings.setValue( "loadPathVolume", mSettingsData.loadPathVolume );

    settings.setValue("spinSVCubeness", ui->spinSVCubeness->value());
    settings.setValue("spinSVSeed", ui->spinSVSeed->value());
    settings.setValue( "spinSVZ", ui->spinSVZ->value() );

    qDebug() << m_sSettingsFile;
}

void AnnotatorWnd::actionLoadScoreImageTriggered()
{
    QString fileName = QFileDialog::getOpenFileName( this, "Load score image", mSettingsData.loadPathScores, mFileTypeFilter );

    if (fileName.isEmpty())
        return;

    qDebug() << fileName;

    std::string stdFName = fileName.toLocal8Bit().constData();

    if (!mScoreImage.load( stdFName ))
        QMessageBox::critical(this, "Cannot open file", QString("%1 could not be read.").arg(fileName));


    if ( !mScoreImage.isSizeLike( mVolumeData ) )
    {
        QMessageBox::critical(this, "Dimensions do not match", "Score image does not match original volume dimensions. Disabling score visualization.");

        mScoreImageEnabled = false;
        ui->actionScoreImageEnabled->setChecked(mScoreImageEnabled);
        ui->chkConnectivityScoreImg->setEnabled(mScoreImageEnabled);
        ui->chkScoreEnable->setEnabled(mScoreImageEnabled);

        updateImageSlice();
        return;
    }

    // enable and show ;)
    mScoreImageEnabled = true;
    ui->actionScoreImageEnabled->setChecked(mScoreImageEnabled);
    ui->chkConnectivityScoreImg->setEnabled(mScoreImageEnabled);
    ui->chkScoreEnable->setEnabled(mScoreImageEnabled);

    updateImageSlice();
    statusBarMsg("Score image loaded successfully.");

    mSettingsData.loadPathScores = QFileInfo(fileName).absolutePath();
    this->saveSettings();
}

void AnnotatorWnd::actionEnableScoreImageTriggered()
{
    if ( !mScoreImage.isSizeLike( mVolumeData ) ) {
        ui->actionScoreImageEnabled->setChecked(false);
        return;
    }

    mScoreImageEnabled = ui->actionScoreImageEnabled->isChecked();
    updateImageSlice();
}

void AnnotatorWnd::actionSaveAnnotTriggered()
{
    QString fileName = QFileDialog::getSaveFileName( this, "Save annotation", mSettingsData.savePath, mFileTypeFilter );

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

    mSettingsData.savePath = QFileInfo(fileName).absolutePath();
    this->saveSettings();
}

void AnnotatorWnd::actionLoadAnnotTriggered()
{
    QString fileName = QFileDialog::getOpenFileName( this, "Load annotation", mSettingsData.loadPath, mFileTypeFilter );

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

    mSettingsData.loadPath = QFileInfo(fileName).absolutePath();
    this->saveSettings();
}

Region3D AnnotatorWnd::getViewportRegion3D()
{
    // prepare Z range
    int zMin = mCurZSlice - ui->spinSVZ->value();
    int zMax = mCurZSlice + ui->spinSVZ->value();

    if (zMin < 0)   zMin = 0;
    if (zMax >= mVolumeData.depth())    zMax = mVolumeData.depth() - 1;

    // selected x,y region + whole z range
    return Region3D( ui->labelImg->getViewableRect(), zMin, zMax - zMin );
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
    mSVRegion = getViewportRegion3D();

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

    // save supervoxel parameters
    saveSettings();
}

void AnnotatorWnd::runConnectivityCheck( const Region3D &reg )
{
    const bool showInfo = ui->chkShowRegionInfo->isChecked();

    if ( reg.valid )
    {
        Matrix3D<LabelType> croppedImg;
        if ( !ui->chkConnectivityScoreImg->isChecked() )    // score image or label image?
            reg.useToCrop( mVolumeLabels, &croppedImg );
        else
            reg.useToCrop( mScoreImage, &croppedImg );

        // now only keep the label set in the combo box in the case of label image
        const LabelType lbl = ui->comboLabel->currentIndex();

        unsigned int lblCount;

        // score image or label image?
        PixelType minThr = lbl; PixelType maxThr = lbl;
        if ( ui->chkConnectivityScoreImg->isChecked() ) {
            minThr = ui->spinScoreThreshold->value();
            maxThr = 255;
        }
        croppedImg.createLabelMap( minThr, maxThr,
                                   &mLabelListData.lblMatrix, ui->chkConnectivityfull->isChecked(), &lblCount, showInfo?(&mLabelListData.shapeInfo):0,
                                   ui->chkConnectivityDetailedAnalysis->isChecked());

        mLabelListData.lblCount = lblCount;

        // save the region for which this was computed for offset computation later on
        mLabelListData.region3D = reg;

        if (showInfo && (lblCount > 0))
        {
            // sort by size
            std::sort( mLabelListData.shapeInfo.begin(), mLabelListData.shapeInfo.end(), ShapeStatistics<>::greaterThan );

            if (mLabelListData.pFrame != 0)
                delete mLabelListData.pFrame;

            mLabelListData.pFrame = new RegionListFrame;

            connect( mLabelListData.pFrame, SIGNAL(currentRegionChanged(int)),
                     this, SLOT(regionListFrameIndexChanged(int)) );

            mLabelListData.pFrame->setRegionData( mLabelListData.shapeInfo );

            mLabelListData.pFrame->show();
            mLabelListData.pFrame->moveToBottomLeftCorner();
        }

        // add to current message
        QString curMsg = this->statusBar()->currentMessage();
        if (!curMsg.isEmpty())
            curMsg += " | ";

        statusBarMsg( curMsg + QString("Region count: %1").arg(lblCount), 2000 );
    }
}

void AnnotatorWnd::regionListFrameIndexChanged(int newRegionIdx)
{
    //qDebug() << newRegionIdx;
    // find pixels and set as highlighted supervoxel
    std::vector<unsigned int>   regionIdxs;
    mLabelListData.lblMatrix.findPixelWithvalue( mLabelListData.shapeInfo[newRegionIdx].labelIdx() , regionIdxs );

    PixelInfoList  pixList;

    for (unsigned int i=0; i < regionIdxs.size(); i++)
    {
        unsigned int x,y,z;
        unsigned int idx = regionIdxs[i];
        mLabelListData.lblMatrix.idxToCoord( idx, x, y, z );

        pixList.push_back( PixelInfo(x,y,z,idx) );
    }

    // convert list to whole image coords
    mLabelListData.region3D.croppedToWholePixList( mLabelListData.lblMatrix, pixList, mSelectedSV.pixelList );

    // compute centroid so that we can focus on the area we want
    UIntPoint3D centerPix;    // will hold centroid
    centerPix.x = centerPix.y = centerPix.z = 0;
    for (unsigned int i=0; i < mSelectedSV.pixelList.size(); i++)
        centerPix.add( mSelectedSV.pixelList[i].coords );

    centerPix.divideBy( mSelectedSV.pixelList.size() );

    ui->zSlider->setValue( centerPix.z );

    mSelectedSV.valid = true;

    updateImageSlice();
}

void AnnotatorWnd::butRunConnectivityCheckNowClicked()
{
    Region3D reg = getViewportRegion3D();
    runConnectivityCheck( reg );
}

void AnnotatorWnd::userModifiedSupervoxelLabel()
{
    if ( ui->chkConnectivityEnableOnline->isChecked() )
        butRunConnectivityCheckNowClicked();
}

void AnnotatorWnd::annotVis3DClicked()
{
    Region3D reg = getViewportRegion3D();

    // warn the user about memory usage
    {
        const double memUsg = (reg.totalVoxels() * 3) / (1024.0*1024.0);
        QMessageBox::StandardButton res =
                QMessageBox::question( this, "Memory usage",
                               QString("This operation needs %1 MB of RAM/hard drive space. Continue?").arg(memUsg),
                               QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );

        if (res != QMessageBox::Yes)
            return;
    }

    if ( reg.valid )
    {
        FijiConfig  cfg;
        cfg.fijiExe = "/data/phd/software/Fiji.app/fiji-linux64";

        Matrix3D<LabelType> lblCropped;
        reg.useToCrop( mVolumeLabels, &lblCropped );

        Matrix3D<PixelType> vR, vG, vB;
        vR.reallocSizeLike( lblCropped );
        vG.reallocSizeLike( lblCropped );   // alloc for each color channel
        vB.reallocSizeLike( lblCropped );

        vR.fill(0); vG.fill(0); vB.fill(0);

        const unsigned int N = lblCropped.numElem();
        const unsigned int maxLbl = mLblColorList.colorList.size();
        for (unsigned int i=0; i < N; i++)
        {
            LabelType lbl = lblCropped.data()[i];
            if (lbl <= 0)   continue;

            if (lbl > maxLbl) continue;

            vR.data()[i] = mLblColorList.colorList[lbl-1].red();
            vG.data()[i] = mLblColorList.colorList[lbl-1].green();
            vB.data()[i] = mLblColorList.colorList[lbl-1].blue();
        }

        FijiShow3D  s3d;
        s3d.setConfig(cfg);
        s3d.run( vR, vG, vB );
    }
}

void AnnotatorWnd::statusBarMsg( const QString &str, int timeout )
{
    statusBar()->showMessage( str, timeout );
}

void AnnotatorWnd::fillLabelComboBox( int numLabels )
{
    if (numLabels > mLblColorList.colorList.size()) {
        qWarning("fillLabelComboBox: numLabels exceeds hueList size");
        numLabels = mLblColorList.colorList.size();
    }

    ui->comboLabel->clear();

    // first none class
    ui->comboLabel->addItem("Unlabeled");

    for (int i=0; i < mLblColorList.colorList.size(); i++)
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

    // score overlay?
    if ( mScoreImageEnabled )
    {
        // sanity check
        if ( !mScoreImage.isSizeLike( mVolumeData ) ) {
            qWarning("mScoreImageEnabled == true but score image is not of valid size");
        }
        else
        {
            // use as red channel
            const PixelType *scorePtr = mScoreImage.sliceData( mCurZSlice );

            unsigned int *pixPtr = (unsigned int *) qimg.constBits(); // trick!

            unsigned int sz = mVolumeLabels.width() * mVolumeLabels.height();

            bool first = true;
            for (unsigned int i=0; i < sz; i++)
            {
                float r = (pixPtr[i] >> 16) & 0xFF;
                float g = (pixPtr[i] >> 8) & 0xFF;
                float b = (pixPtr[i] >> 0) & 0xFF;

                float sc = scorePtr[i] / 255.0f;
                float scInv = 1.0f - sc;

                if ( first && (sc > 0.8) )
                    qDebug("Bef: %f %f %f", r, g ,b);

                g = scInv*g;
                b = scInv*b;
                r = r * scInv +  sc*255;

                unsigned int iR = r;
                unsigned int iG = g;
                unsigned int iB = b;

                if (first && (sc > 0.8)) {
                    qDebug("Val: %f %f %f %f", r, g, b, sc);
                    first = false;
                }

                //pixPtr[i] = (pixPtr[i] & 0xFF00FFFF) | (((unsigned int) scorePtr[i]) << 16);
                pixPtr[i] = 0xFF000000 | (iR<<16) | (iG<<8) | iB;
            }
        }
    }

    // check ground truth slice and draw it
    if (mOverlayLabelImage)
    {
        //int intTransp = 255*mOverlayLabelImageTransparency;
        int floatTransp = 256 * mOverlayLabelImageTransparency;
        int floatTranspInv = 256 - floatTransp;

        const LabelType *lblPtr = mVolumeLabels.sliceData( mCurZSlice );
        unsigned int *pixPtr = (unsigned int *) qimg.constBits(); // trick!

        unsigned int sz = mVolumeLabels.width() * mVolumeLabels.height();
        int maxLabel = (int) mLblColorList.colorList.size();

        for (unsigned int i=0; i < sz; i++)
        {
            if ( lblPtr[i] == 0 )
                continue;   // unlabeled, we don't care

            if (lblPtr[i] > maxLabel) {
                qDebug("Label exceed possible colors: %d", (int)lblPtr[i]);
                continue;
            }

            int vv = (floatTransp * (pixPtr[i] & 0xFF))/256;

            const QColor &color = mLblColorList.colorList[ lblPtr[i] - 1];

            int g = vv + (floatTranspInv * color.green()) / 256;
            int b = vv + (floatTranspInv * color.blue()) / 256;
            int r = vv + (floatTranspInv * color.red()) / 256;

            QColor c = QColor::fromRgb( r,g,b );

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

    // callback
    userModifiedSupervoxelLabel();
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

        if(0)   // try region growing
        {
            // compute region mean
            unsigned int mean = 0;
            for ( unsigned int i=0; i < mSelectedSV.pixelList.size(); i++ )
                mean += mVolumeData.data()[ mSelectedSV.pixelList[i].index ];

            mean /= mSelectedSV.pixelList.size();
            double fMean = mean;

            double var = 0;
            for ( unsigned int i=0; i < mSelectedSV.pixelList.size(); i++ ) {
                double dv = mVolumeData.data()[mSelectedSV.pixelList[i].index] - fMean;
                var += dv*dv;
            }

            double stdDev = sqrt( var / mSelectedSV.pixelList.size() );

            double minVal = fMean - stdDev;
            double maxVal = fMean + stdDev;

            if ( minVal < 0 )   minVal = 0;
            if ( maxVal > 255 )   maxVal = 255;
            qDebug() << "Limits " << minVal << "  " << maxVal;

            PixelInfoList pixListResult;
            unsigned int maxRegionSize = 4 * mSelectedSV.pixelList.size();
            RegionGrow<PixelType>( mVolumeData, mSelectedSV.pixelList, (PixelType)minVal, (PixelType)maxVal, maxRegionSize,  &pixListResult );

            qDebug() << "New: " << pixListResult.size();

            mSelectedSV.pixelList = pixListResult;
        }

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
        unsigned char thrMin = ui->spinPixMin->value();
        unsigned char thrMax = ui->spinPixMax->value();
        // make copy
        SlicMapType::value_type oldList = mSelectedSV.pixelList;

        const bool dontOverwriteLabeledPixs = ui->chkDontOverwriteLabeledPIxs->isChecked();

        mSelectedSV.pixelList.clear();

        for (int i=0; i < (int)oldList.size(); i++)
        {
            PixelType val = mVolumeData.data()[ oldList[i].index ];

            if ( (val < thrMin) || (val > thrMax) )
                continue;   //ignore

            if ( dontOverwriteLabeledPixs ) {
                bool alreayLabeled = mVolumeLabels.data()[ oldList[i].index ] != 0;
                if ( alreayLabeled )
                    continue;
            }

            mSelectedSV.pixelList.push_back( oldList[i] );
        }
    }

    // if score image present + score thresholding selected
    if ( ui->chkScoreEnable->isChecked() && mScoreImage.isSizeLike(mVolumeData) )
    {
        // make copy
        SlicMapType::value_type oldList = mSelectedSV.pixelList;

        mSelectedSV.pixelList.clear();

        unsigned char thrVal = ui->spinScoreThreshold->value();

        for (int i=0; i < (int)oldList.size(); i++)
        {
            PixelType val = mScoreImage.data()[ oldList[i].index ];

            if ( val < thrVal )
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
            statusBarMsg( QString().sprintf("Zoom: %.1f", ui->labelImg->scaleFactor()) );

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

void AnnotatorWnd::pluginUpdateDisplay()
{
    updateImageSlice();
}

QMenu *AnnotatorWnd::getPluginMenuPtr()
{
    return ui->menuPlugins;
}

AnnotatorWnd::~AnnotatorWnd()
{
    delete ui;
}


