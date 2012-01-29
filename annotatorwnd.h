#ifndef ANNOTATORWND_H
#define ANNOTATORWND_H

#include <QMainWindow>
#include "Matrix3D.h"
#include "ColorLists.h"

#include "Region3D.h"
#include "CommonTypes.h"

namespace Ui {
    class AnnotatorWnd;
}

struct SupervoxelSelection;

class AnnotatorWnd : public QMainWindow
{
    Q_OBJECT

public:
    explicit AnnotatorWnd(QWidget *parent = 0);
    ~AnnotatorWnd();

private:
    // SETTINGS
    QString m_sSettingsFile;

    struct {
        QString savePath;
        QString loadPath;   // these are for annotation

        QString loadPathScores; // for score volume
        QString loadPathVolume; // for main volume
    } mSettingsData;

    void loadSettings();
    void saveSettings();

private:
    Ui::AnnotatorWnd *ui;
    int mCurZSlice;

    QString mFileTypeFilter;

    // colors for labels + icons
    LabelColorList   mLblColorList;
    OverlayColorList mOverlayColorList;
    SelectionColor   mSelectionColor;
    ScoreColor       mScoreColor;

    // returns a region3d object according to current viewport
    //  and the slice spin box in the form
    Region3D getViewportRegion3D();

    Matrix3D<PixelType>  mVolumeData;    // loaded data (volume), whole volume
    Matrix3D<LabelType>  mVolumeLabels;  // labels for each pixel in original volume

    Matrix3D<PixelType>  mCroppedVolumeData;    // cropped version of data, to keep sizes manageable


    // score image (if loaded), only for aid in labeling
    Matrix3D<ScoreType> mScoreImage;
    bool                mScoreImageEnabled;



    void updateCursorPixelInfo( int x, int y, int z );   // shows current pixel position

    bool   mOverlayLabelImage;    // if true, then an overlay is drawn on top of the image, showing color-coded pixel labels
    double mOverlayLabelImageTransparency;  // btw 0 and 1, transparency of labels

    void statusBarMsg( const QString &str, int timeout = 1200 );

    void annotateSupervoxel( const SupervoxelSelection &SV, LabelType label );

    // scans for plugins and adds them.
    void scanPlugins( const QString &pluginFolder );


    void runConnectivityCheck( const Region3D &reg );

public:

    // called by the plugin to update the display
    void pluginUpdateDisplay();

    QMenu *getPluginMenuPtr();

    // more for plugins
    Matrix3D<PixelType> &   getVolumeVoxelData() {  return mVolumeData; }
    Matrix3D<LabelType> &   getLabelVoxelData()  {  return mVolumeLabels; }
    Matrix3D<ScoreType> &   getScoreVoxelData()  {  return mScoreImage; }

    Matrix3D<OverlayType> & getOverlayVoxelData( unsigned int num );
    void                    setOverlayVisible( unsigned int num, bool visible );

    // returns one string per class name
    void                    getLabelClassList( QStringList &sList );

public slots:

    void updateImageSlice();    //updates the label widget with mCurZSlice slice

    // called whenever the user has modified a single label supervoxel
    void userModifiedSupervoxelLabel();
    void butRunConnectivityCheckNowClicked();

    void labelImageWheelEvent(QWheelEvent * e);
    void labelImageMouseMoveEvent(QMouseEvent * e);
    void labelImageMouseReleaseEvent(QMouseEvent * e);

    void zSliderMoved( int );

    void chkLabelOverlayStateChanged(int state);
    void actionLabelOverlayTriggered();

    void annotVis3DClicked();

    void dialOverlayTransparencyMoved( int );

    // fills the labels combo box with the requested number of labels + the 'unlabeled' one
    void fillLabelComboBox( int numLabels );

    void genSupervoxelClicked();

    void actionSaveAnnotTriggered();
    void actionLoadAnnotTriggered();

    void actionLoadScoreImageTriggered();
    void actionEnableScoreImageTriggered();

    // called when the region changes in the region info window
    void regionListFrameIndexChanged(int);
    void regionListFrameLabelRegion(uint,uint);

    // called when an image wants to be loaded in an overlay layer
    void overlayLoadTriggered();
    void overlayChooseColorTriggered();
};

#endif // ANNOTATORWND_H
