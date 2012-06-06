//Our includes
#include "mygraphicsview.h"

//Qt includes
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTextStream>
#include <QScrollBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <QTime>

void MyGraphicsView::setImage( const QImage &img, const QRect &updateRect )
{
    const bool firstTime = mScene->items().size() == 0;

    //Set-up the view if it is the 1st time
    if (firstTime)
    {
        setSceneRect(0, 0, img.width(), img.height());
        SetCenter(QPointF(img.width()/2.0, img.height()/2.0)); //A modified version of centerOn(), handles special cases

        mScene->addPixmap( QPixmap() );
    }


    ((QGraphicsPixmapItem *) mScene->items()[0])->setPixmap( QPixmap::fromImage( img ) );

    //qDebug("Valid: %d", (int)updateRect.isValid());

    QRect toUpdate;
    if (updateRect.isValid())
        toUpdate = updateRect;
    else
        toUpdate = img.rect();

    //toUpdate.setHeight(100);
    //toUpdate.setWidth(100);

    QRect thisToScreen = mapFromScene( toUpdate ).boundingRect();

    QRect newUpdate = thisToScreen.intersect( viewport()->rect() );

    viewport()->update( newUpdate );

    //mScene->update( QRectF(0,0,10,10) );

    //qDebug("Image: %d %d", img.size().width(), img.size().height() );
    //mScene->addPixmap( mPixmap );

    /*mScene->update();
    update();*/
}

/**
* Sets up the subclassed QGraphicsView
*/
MyGraphicsView::MyGraphicsView(QWidget* parent) : QGraphicsView(parent)
{
    mScaleFactor = 1.0;
    setZoomLimits( 0.1, 7 );

    setRenderHints(0);
    //setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    //Set-up the scene
    mScene = new QGraphicsScene(this);
    setScene(mScene);

    this->setCacheMode( QGraphicsView::CacheNone );

    this->setViewportUpdateMode( QGraphicsView::NoViewportUpdate);
}

/**
  * Sets the current centerpoint.  Also updates the scene's center point.
  * Unlike centerOn, which has no way of getting the floating point center
  * back, SetCenter() stores the center point.  It also handles the special
  * sidebar case.  This function will claim the centerPoint to sceneRec ie.
  * the centerPoint must be within the sceneRec.
  */
//Set the current centerpoint in the
void MyGraphicsView::SetCenter(const QPointF& centerPoint)
{
    //Get the rectangle of the visible area in scene coords
    QRectF visibleArea = mapToScene(rect()).boundingRect();

    //Get the scene area
    QRectF sceneBounds = sceneRect();

    double boundX = visibleArea.width() / 2.0;
    double boundY = visibleArea.height() / 2.0;
    double boundWidth = sceneBounds.width() - 2.0 * boundX;
    double boundHeight = sceneBounds.height() - 2.0 * boundY;

    //The max boundary that the centerPoint can be to
    QRectF bounds(boundX, boundY, boundWidth, boundHeight);

    if(bounds.contains(centerPoint)) {
        //We are within the bounds
        CurrentCenterPoint = centerPoint;
    } else {
        //We need to clamp or use the center of the screen
        if(visibleArea.contains(sceneBounds)) {
            //Use the center of scene ie. we can see the whole scene
            CurrentCenterPoint = sceneBounds.center();
        } else {

            CurrentCenterPoint = centerPoint;

            //We need to clamp the center. The centerPoint is too large
            if(centerPoint.x() > bounds.x() + bounds.width()) {
                CurrentCenterPoint.setX(bounds.x() + bounds.width());
            } else if(centerPoint.x() < bounds.x()) {
                CurrentCenterPoint.setX(bounds.x());
            }

            if(centerPoint.y() > bounds.y() + bounds.height()) {
                CurrentCenterPoint.setY(bounds.y() + bounds.height());
            } else if(centerPoint.y() < bounds.y()) {
                CurrentCenterPoint.setY(bounds.y());
            }

        }
    }

    //Update the scrollbars
    centerOn(CurrentCenterPoint);
}

#if 0
/**
  * Handles when the mouse button is pressed
  */
void MyGraphicsView::mousePressEvent(QMouseEvent* event) {
    //For panning the view
    LastPanPoint = event->pos();
    setCursor(Qt::ClosedHandCursor);
}

/**
  * Handles when the mouse button is released
  */
void MyGraphicsView::mouseReleaseEvent(QMouseEvent* event) {
    setCursor(Qt::OpenHandCursor);
    LastPanPoint = QPoint();
}

/**
*Handles the mouse move event
*/
void MyGraphicsView::mouseMoveEvent(QMouseEvent* event) {
    if(!LastPanPoint.isNull()) {
        //Get how much we panned
        QPointF delta = mapToScene(LastPanPoint) - mapToScene(event->pos());
        LastPanPoint = event->pos();

        //Update the center ie. do the pan
        SetCenter(GetCenter() + delta);
    }
}

/**
  * Zoom the view in and out.
  */
void MyGraphicsView::wheelEvent(QWheelEvent* event) {

    //Get the position of the mouse before scaling, in scene coords
    QPointF pointBeforeScale(mapToScene(event->pos()));

    //Get the original screen centerpoint
    QPointF screenCenter = GetCenter(); //CurrentCenterPoint; //(visRect.center());

    //Scale the view ie. do the zoom
    double scaleFactor = 1.15; //How fast we zoom
    if(event->delta() > 0) {
        //Zoom in
        scale(scaleFactor, scaleFactor);
    } else {
        //Zooming out
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }

    //Get the position after scaling, in scene coords
    QPointF pointAfterScale(mapToScene(event->pos()));

    //Get the offset of how the screen moved
    QPointF offset = pointBeforeScale - pointAfterScale;

    //Adjust to the new center for correct zooming
    QPointF newCenter = screenCenter + offset;
    SetCenter(newCenter);
}
#endif

void MyGraphicsView::scale(double factor)
{
    //Get the position of the mouse before scaling, in scene coords
    QPointF pointBeforeScale(mapToScene( mLastMouseMovePt ));

    //Get the original screen centerpoint
    QPointF screenCenter = GetCenter(); //CurrentCenterPoint; //(visRect.center());

    double tempScale = factor;
    tempScale *= mScaleFactor;

    if ( tempScale > mZoomMax )
        tempScale = mZoomMax;
    if (tempScale < mZoomMin)
        tempScale = mZoomMin;

    factor = tempScale / mScaleFactor;

    mScaleFactor = tempScale;

    //Scale the view ie. do the zoom
    QGraphicsView::scale(factor, factor);

    //Get the position after scaling, in scene coords
    QPointF pointAfterScale(mapToScene( mLastMouseMovePt ));

    //Get the offset of how the screen moved
    QPointF offset = pointBeforeScale - pointAfterScale;

    //Adjust to the new center for correct zooming
    QPointF newCenter = screenCenter + offset;
    SetCenter(newCenter);
}


void MyGraphicsView::pan( double x, double y )
{
    QPointF delta( x, y );
    SetCenter(GetCenter() + delta);
    viewport()->update();
}

/**
  * Need to update the center so there is no jolt in the
  * interaction after resizing the widget.
  */
void MyGraphicsView::resizeEvent(QResizeEvent* event) {
    //Get the rectangle of the visible area in scene coords
    QRectF visibleArea = mapToScene(rect()).boundingRect();
    SetCenter(visibleArea.center());

    //Call the subclass resize so the scrollbars are updated correctly
    QGraphicsView::resizeEvent(event);
}

QRect MyGraphicsView::getViewableRect() const
{
    if (mScene->items().size() == 0)
        return QRect();

    QRect r =
            mapToScene( viewport()->geometry() ).boundingRect().toRect();

    //qDebug() << "Rect " << r;

    if ( r.top() < 0 )
        r.setTop(0);

    if ( r.left() < 0 )
        r.setLeft(0);

    // get only intersection between pixmap and region
    r = r.intersect( mScene->items()[0]->boundingRect().toRect() );

    //qDebug() << "Rect2 " << r;

    return r;
}
