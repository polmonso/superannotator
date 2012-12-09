#ifndef GRAPHCUTSPLUGIN_H
#define GRAPHCUTSPLUGIN_H

/** We shouldn't need a .h file
  * but it is necessary for the Q_OBJECT correctness (vtable)
  */

#include <PluginBase.h>
#include <cstdlib>
#include <QMessageBox>
#include <QMouseEvent>

using namespace std;

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
            QAction *action = mPluginServices->getPluginMenu()->addAction( QString("Run %1").arg(i+1) );
            action->setData( i );
            connect( action, SIGNAL(triggered()), this, SLOT(runGraphCuts()) );
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
        if ( (evt->buttons() & Qt::LeftButton) == 0)
           return;

        printf("%d %d %d\n", imgX, imgY, imgZ);

        Matrix3D<ScoreType> &scoreMatrix = mPluginServices->getOverlayVolumeData(0);

        if (scoreMatrix.isEmpty())
        {
            scoreMatrix.reallocSizeLike( mPluginServices->getVolumeVoxelData() );
            scoreMatrix.fill(0);
        }

        if (evt->modifiers() & Qt::ShiftModifier) {
            for(int x = max(0, (int)imgX - 1); x <= min(scoreMatrix.width()-1, imgX + 1); ++x) {
                for(int y = max(0, (int)imgY - 1); y <= min(scoreMatrix.height()-1, imgY + 1); ++y) {
                    for(int z = max(0, (int)imgZ - 1); z <= min(scoreMatrix.depth()-1, imgZ + 1); ++z) {
                        scoreMatrix(x,y,z) = 128;
                    }
                }
            }
           // scoreMatrix( imgX, imgY, imgZ ) = 128;
        }
        else {
            for(int x = max(0, (int)imgX - 1); x <= min(scoreMatrix.width()-1, imgX + 1); ++x) {
                for(int y = max(0, (int)imgY - 1); y <= min(scoreMatrix.height()-1, imgY + 1); ++y) {
                    for(int z = max(0, (int)imgZ - 1); z <= min(scoreMatrix.depth()-1, imgZ + 1); ++z) {
                        scoreMatrix(x,y,z) = 255;
                    }
                }
            }

            //scoreMatrix( imgX, imgY, imgZ ) = 255;
        }

        mPluginServices->setOverlayVisible( 0, true );
        mPluginServices->updateDisplay();
    }

public slots:

    void runGraphCuts();

    void showMsgBoxClicked()
    {
        QMessageBox::information( mPluginServices->getMainWindow(), "Clicked me!", "You have just clicked me." );
    }
};

#endif // GRAPHCUTSPLUGIN_H
