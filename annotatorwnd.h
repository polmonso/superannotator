#ifndef ANNOTATORWND_H
#define ANNOTATORWND_H

#include <QMainWindow>
#include "Matrix3D.h"
#include "ColorLists.h"

#include "Region3D.h"

namespace Ui {
    class AnnotatorWnd;
}

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

    // returns a region3d object according to current viewport
    //  and the slice spin box in the form
    Region3D getViewportRegion3D();

    typedef unsigned char PixelType;
    typedef unsigned char LabelType;

    Matrix3D<PixelType>  mVolumeData;    // loaded data (volume), whole volume
    Matrix3D<LabelType>  mVolumeLabels;  // labels for each pixel in original volume

    Matrix3D<PixelType>  mCroppedVolumeData;    // cropped version of data, to keep sizes manageable


    // score image (if loaded), only for aid in labeling
    Matrix3D<PixelType> mScoreImage;
    bool                mScoreImageEnabled;


    void updateImageSlice();    //updates the label widget with mCurZSlice slice

    void updateCursorPixelInfo( int x, int y, int z );   // shows current pixel position

    bool   mOverlayLabelImage;    // if true, then an overlay is drawn on top of the image, showing color-coded pixel labels
    double mOverlayLabelImageTransparency;  // btw 0 and 1, transparency of labels

    void statusBarMsg( const QString &str, int timeout = 1200 );

    void annotateSelectedSupervoxel();


    void runConnectivityCheck( const Region3D &reg );

public slots:
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
};

#endif // ANNOTATORWND_H
