#ifndef ANNOTATORWND_H
#define ANNOTATORWND_H

#include <QMainWindow>
#include "Matrix3D.h"
#include "ColorLists.h"

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
    Ui::AnnotatorWnd *ui;
    int mCurZSlice;

    // colors for labels + icons
    LabelColorList   mLblColorList;

    typedef unsigned char PixelType;
    typedef unsigned char LabelType;

    Matrix3D<PixelType>  mVolumeData;    // loaded data (volume), whole volume
    Matrix3D<LabelType>  mVolumeLabels;  // labels for each pixel in original volume

    Matrix3D<PixelType>  mCroppedVolumeData;    // cropped version of data, to keep sizes manageable


    void updateImageSlice();    //updates the label widget with mCurZSlice slice

    void updateCursorPixelInfo( int x, int y, int z );   // shows current pixel position

    bool   mOverlayLabelImage;    // if true, then an overlay is drawn on top of the image, showing color-coded pixel labels
    double mOverlayLabelImageTransparency;  // btw 0 and 1, transparency of labels

    void statusBarMsg( const QString &str, int timeout = 1200 );

public slots:
    void labelImageWheelEvent(QWheelEvent * e);
    void labelImageMouseMoveEvent(QMouseEvent * e);
    void labelImageMouseReleaseEvent(QMouseEvent * e);

    void zSliderMoved( int );

    void chkLabelOverlayStateChanged(int state);
    void actionLabelOverlayTriggered();

    void dialOverlayTransparencyMoved( int );

    // fills the labels combo box with the requested number of labels + the 'unlabeled' one
    void fillLabelComboBox( int numLabels );

    void genSupervoxelClicked();

    void actionSaveAnnotTriggered();
};

#endif // ANNOTATORWND_H
