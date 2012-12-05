#ifndef GRAPHCUTSPLUGIN_H
#define GRAPHCUTSPLUGIN_H

/** We shouldn't need a .h file
  * but it is necessary for the Q_OBJECT correctness (vtable)
  */

#include <PluginBase.h>
#include <cstdlib>
#include <QMessageBox>
#include <QMouseEvent>

class GraphCutsPlugin : public PluginBase
{
    Q_OBJECT
private:
    const PluginServices* mPluginServices;

public:
    GraphCutsPlugin(QObject *parent = 0) : PluginBase(parent) {}

    bool    initializePlugin( const PluginServices &pServices )
    {
        mPluginServices = &pServices;

        /** Add a menu item **/
        for (unsigned int i=0; i < mPluginServices->getMaxOverlayVolumes(); i++) {
            QAction *action = mPluginServices->getPluginMenu()->addAction( QString("Random labels to overlay %1").arg(i+1) );
            action->setData( i );
            connect( action, SIGNAL(triggered()), this, SLOT(randomLabelsClicked()) );
        }

        /** Add a menu item **/
        QAction *action = mPluginServices->getPluginMenu()->addAction( "Show message box" );
        connect( action, SIGNAL(triggered()), this, SLOT(showMsgBoxClicked()) );

        return true;
    }

    // must return the plugin's name
    QString pluginName() {
        return "Test plugin";
    }

    virtual void  mouseMoveEvent( QMouseEvent *evt, unsigned int imgX, unsigned int imgY, unsigned int imgZ )
    {
        if ( (evt->buttons() & Qt::LeftButton) == 0 )
            return;

        Matrix3D<ScoreType> &scoreMatrix = mPluginServices->getOverlayVolumeData(0);

        if (scoreMatrix.isEmpty())
            scoreMatrix.reallocSizeLike( mPluginServices->getVolumeVoxelData() );

        scoreMatrix( imgX, imgY, imgZ ) = 255;

        mPluginServices->setOverlayVisible( 0, true );
        mPluginServices->updateDisplay();
    }

public slots:
    void randomLabelsClicked()
    {
        QAction *action = qobject_cast<QAction *>(sender());
        unsigned int idx = action->data().toUInt();

        qDebug("Overlay %d", idx);

        Matrix3D<OverlayType> &ovMatrix = mPluginServices->getOverlayVolumeData( idx );

        // MUST BE RESIZED!
        ovMatrix.reallocSizeLike( mPluginServices->getVolumeVoxelData() );

        LabelType *dPtr = ovMatrix.data();

        for (unsigned int i=0; i < ovMatrix.numElem(); i++)
            dPtr[i] = rand() % 128;

        // set enabled
        mPluginServices->setOverlayVisible( idx, true );

        mPluginServices->updateDisplay();
    }

    void showMsgBoxClicked()
    {
        QMessageBox::information( mPluginServices->getMainWindow(), "Clicked me!", "You have just clicked me." );
    }
};

#endif // GRAPHCUTSPLUGIN_H
