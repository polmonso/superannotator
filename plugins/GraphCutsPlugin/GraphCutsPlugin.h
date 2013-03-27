#ifndef GRAPHCUTSPLUGIN_H
#define GRAPHCUTSPLUGIN_H

/** We shouldn't need a .h file
  * but it is necessary for the Q_OBJECT correctness (vtable)
  */

#include <PluginBase.h>
#include <cstdlib>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTimer>

using namespace std;

const int idx_seed_overlay = 0;
const int idx_bindata_overlay = 1;
const int idx_output_overlay = 3;

enum eGCType
{
    GC_DEFAULT = 0,
    GC_LIMITED,
    GC_SCORES
};

class GraphCutsPlugin : public PluginBase
{
    Q_OBJECT
private:
    const PluginServices* mPluginServices;

    QTimer* timer;

    // id of the active overlay
    int activeOverlay;

    bool mouseEventDetected;

public:
    GraphCutsPlugin(QObject *parent = 0);

    ~GraphCutsPlugin();

    bool    initializePlugin( const PluginServices &pServices )
    {
        mPluginServices = &pServices;

        /** Add a menu item **/
        {
            QAction *action = mPluginServices->getPluginMenu()->addAction( QString("Run") );
            action->setData(GC_DEFAULT);
            connect( action, SIGNAL(triggered()), this, SLOT(runGraphCuts()) );
        }

        /** Add a menu item **/
        {
            QAction *action = mPluginServices->getPluginMenu()->addAction( QString("Run (limited size)") );
            action->setData(GC_LIMITED);
            connect( action, SIGNAL(triggered()), this, SLOT(runGraphCuts()) );
        }

        /** Add a menu item **/
        {
            QAction *action = mPluginServices->getPluginMenu()->addAction( QString("Run (scores)") );
            action->setData(GC_SCORES);
            connect( action, SIGNAL(triggered()), this, SLOT(runGraphCuts()) );
        }

        /** Add a menu item **/
        {
        QAction *action = mPluginServices->getPluginMenu()->addAction( "Clean seed overlay" );
        connect( action, SIGNAL(triggered()), this, SLOT(cleanSeedOverlay()) );
        }

        /** Add a menu item **/
        {
        QAction *action = mPluginServices->getPluginMenu()->addAction( "Transfer overlay" );
        connect( action, SIGNAL(triggered()), this, SLOT(transferOverlay()) );
        }

        /** Add a menu item **/
        {
        QAction *action = mPluginServices->getPluginMenu()->addAction( "Change active overlay" );
        connect( action, SIGNAL(triggered()), this, SLOT(changeActiveOverlay()) );
        }

        /** Add a menu item **/
        {
        QAction *action = mPluginServices->getPluginMenu()->addAction( "Show message box" );
        connect( action, SIGNAL(triggered()), this, SLOT(showMsgBoxClicked()) );
        }

        return true;
    }

    // must return the plugin's name
    QString pluginName() {
        return "GraphCut plugin";
    }

    virtual void  mouseMoveEvent( QMouseEvent *evt, unsigned int imgX, unsigned int imgY, unsigned int imgZ )
    {
        if ( (evt->buttons() & Qt::LeftButton) == 0 && (evt->buttons() & Qt::RightButton) == 0)
           return;

        //printf("mouseMoveEvent %x activeOverlay = %d\n", this, activeOverlay);
        Matrix3D<ScoreType> &activeOverlayMatrix = mPluginServices->getOverlayVolumeData(activeOverlay);

        if (activeOverlayMatrix.isEmpty())
        {
            activeOverlayMatrix.reallocSizeLike( mPluginServices->getVolumeVoxelData() );
            activeOverlayMatrix.fill(0);
        }

        if (evt->buttons() & Qt::RightButton) {
            for(int x = max(0, (int)imgX - 1); x <= min(activeOverlayMatrix.width()-1, imgX + 1); ++x) {
                for(int y = max(0, (int)imgY - 1); y <= min(activeOverlayMatrix.height()-1, imgY + 1); ++y) {
                    for(int z = max(0, (int)imgZ - 1); z <= min(activeOverlayMatrix.depth()-1, imgZ + 1); ++z) {
                        activeOverlayMatrix(x,y,z) = 0;
                    }
                }
            }
           // scoreMatrix( imgX, imgY, imgZ ) = 0;
        } else {
            if (evt->modifiers() & Qt::ShiftModifier) {
                for(int x = max(0, (int)imgX - 1); x <= min(activeOverlayMatrix.width()-1, imgX + 1); ++x) {
                    for(int y = max(0, (int)imgY - 1); y <= min(activeOverlayMatrix.height()-1, imgY + 1); ++y) {
                        for(int z = max(0, (int)imgZ - 1); z <= min(activeOverlayMatrix.depth()-1, imgZ + 1); ++z) {
                            activeOverlayMatrix(x,y,z) = 128;
                        }
                    }
                }
               // scoreMatrix( imgX, imgY, imgZ ) = 128;
            }
            else {
                for(int x = max(0, (int)imgX - 1); x <= min(activeOverlayMatrix.width()-1, imgX + 1); ++x) {
                    for(int y = max(0, (int)imgY - 1); y <= min(activeOverlayMatrix.height()-1, imgY + 1); ++y) {
                        for(int z = max(0, (int)imgZ - 1); z <= min(activeOverlayMatrix.depth()-1, imgZ + 1); ++z) {
                            activeOverlayMatrix(x,y,z) = 255;
                        }
                    }
                }

                //scoreMatrix( imgX, imgY, imgZ ) = 255;
            }
        }

        mouseEventDetected = true;
        //mPluginServices->setOverlayVisible(activeOverlay, true );
        //mPluginServices->updateDisplay();
    }

public slots:

    void changeActiveOverlay();

    void cleanSeedOverlay();

    void runGraphCuts();

    // transfer overlay created by plugin to input overlay
    void transferOverlay();

    void showMsgBoxClicked()
    {
        QMessageBox::information( mPluginServices->getMainWindow(), "Clicked me!", "You have just clicked me." );
    }

    void updateOverlay()
    {
        if(mouseEventDetected) {
            mouseEventDetected = false;
            mPluginServices->setOverlayVisible(activeOverlay, true );
            mPluginServices->updateDisplay();
        }
    }
};

#endif // GRAPHCUTSPLUGIN_H
