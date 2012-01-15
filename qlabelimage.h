#ifndef QLABELIMAGE_H
#define QLABELIMAGE_H

#include <QLabel>
#include <QMouseEvent>
#include <QPoint>
#include <QScrollArea>
#include <QScrollBar>
#include <QRectF>
#include <QPointF>
#include <QSizeF>

class QLabelImage : public QLabel
{
    Q_OBJECT
private:
    double mScaleFactor;
    QScrollArea *mScrollArea;    //pointer to parent?

    double  mZoomMax, mZoomMin; // max and min zoom factor

private:
     inline void adjustScrollBar(QScrollBar *scrollBar, double factor)
     {
         scrollBar->setValue(int(factor * scrollBar->value()
                                 + ((factor - 1) * scrollBar->pageStep()/2)));
     }

public:
    explicit QLabelImage(QWidget *parent = 0);

    void setZoomLimits( double min, double max) { mZoomMax = max; mZoomMin = min; }
    void setScrollArea( QScrollArea *sa ) { mScrollArea = sa; }

    inline void setImage( const QImage &img ) {
        this->setPixmap( QPixmap::fromImage(img) );
    }

    // returns 'viewable rect' in image (pixmap) coordinates
    inline QRect getViewableRect() const
    {
        QSizeF viewportSize = QSizeF(mScrollArea->maximumViewportSize());

        //qDebug("Viewport size: %f %f", viewportSize.width(), viewportSize.height());

        QRectF myCoords = QRectF(this->rect());
        myCoords.setX( this->x() );
        myCoords.setY( this->y() );

        QPointF corner(0,0);
        QSizeF  size(0,0);

        //qDebug("Scale: %f", (float)mScaleFactor);
        //qDebug("x: %f",myCoords.x());
        //qDebug("y: %f",myCoords.y());

        if (myCoords.x() < 0) {
            corner.setX( -myCoords.x() / mScaleFactor );
            size.setWidth( viewportSize.width() / mScaleFactor );
        }

        size.setWidth( viewportSize.width() / mScaleFactor );

        if (myCoords.y() < 0) {
            corner.setY( -myCoords.y() / mScaleFactor );
        }

        size.setHeight( viewportSize.height() / mScaleFactor );

        if (size.width() > pixmap()->width())
            size.setWidth( pixmap()->width() );

        if (size.height() > pixmap()->height())
            size.setHeight( pixmap()->height() );

        return QRect( corner.toPoint(), size.toSize() );
    }

    void scaleImage(double factor, bool factorIsRelative = true)
    {
     Q_ASSERT(pixmap());

     qDebug("Scale: %f", (float)factor);

     double tempScale = factor;
     if (factorIsRelative)
         tempScale *= mScaleFactor;

     if ( tempScale > mZoomMax )
         tempScale = mZoomMax;
     if (tempScale < mZoomMin)
         tempScale = mZoomMin;

     mScaleFactor = tempScale;

     resize(mScaleFactor * pixmap()->size());

     adjustScrollBar(mScrollArea->horizontalScrollBar(), factor);
     adjustScrollBar(mScrollArea->verticalScrollBar(), factor);

     //zoomInAct->setEnabled(scaleFactor < 3.0);
     //zoomOutAct->setEnabled(scaleFactor > 0.333);
    }

    inline QPoint screenToImage( const QPoint &ev ) {
        int x = (int)(ev.x() / mScaleFactor + 0.5);
        int y = (int)(ev.y() / mScaleFactor + 0.5);

        return QPoint(x,y);
    }

protected:
    void wheelEvent ( QWheelEvent * event ) {
        emit wheelEventSignal(event);
    }

    void mouseMoveEvent ( QMouseEvent * ev ) {
        emit mouseMoveEventSignal(ev);
    }

    void mouseReleaseEvent ( QMouseEvent * ev ) {
        emit mouseReleaseEventSignal(ev);
    }

    void mousePressEvent ( QMouseEvent * ev ) {
        emit mousePressEventSignal(ev);
    }

signals:
    void wheelEventSignal( QWheelEvent *event );
    void mouseMoveEventSignal( QMouseEvent *event );
    void mouseReleaseEventSignal( QMouseEvent *event );
    void mousePressEventSignal( QMouseEvent *event );

public slots:
    // fits the image within the scrollarea size
    void zoomFit()
    {
        double ratioSA = mScrollArea->maximumViewportSize().width() * 1.0 / mScrollArea->maximumViewportSize().height();
        double ratioImg = pixmap()->width() * 1.0 / pixmap()->height();

        mScaleFactor = 1.0; //reset
        if (ratioSA < ratioImg)
            scaleImage( mScrollArea->maximumViewportSize().width() * 1.0 / pixmap()->width() );
        else
            scaleImage( mScrollArea->maximumViewportSize().height() * 1.0 / pixmap()->height() );
    }
};

#endif // QLABELIMAGE_H


