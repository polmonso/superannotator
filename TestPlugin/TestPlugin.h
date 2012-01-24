#ifndef TESTPLUGIN_H
#define TESTPLUGIN_H

/** We shouldn't need a .h file
  * but it is necessary for the Q_OBJECT correctness (vtable)
  */

#include <PluginBase.h>
#include <cstdlib>
#include <QMessageBox>

class TestPlugin : public PluginBase
{
    Q_OBJECT
private:
    const PluginServices* mPluginServices;

public:
    TestPlugin(QObject *parent = 0) : PluginBase(parent) {}

    bool    initializePlugin( const PluginServices &pServices )
    {
        mPluginServices = &pServices;

        /** Add a menu item **/
        QAction *action = mPluginServices->getPluginMenu()->addAction( "Random labels" );
        connect( action, SIGNAL(triggered()), this, SLOT(randomLabelsClicked()) );

        /** Add a menu item **/
        action = mPluginServices->getPluginMenu()->addAction( "Show message box" );
        connect( action, SIGNAL(triggered()), this, SLOT(showMsgBoxClicked()) );

        return true;
    }

    // must return the plugin's name
    QString pluginName() {
        return "Test plugin";
    }

public slots:
    void randomLabelsClicked()
    {
        Matrix3D<LabelType> &lblMatrix = mPluginServices->getLabelVoxelData();
        LabelType *dPtr = lblMatrix.data();
        for (unsigned int i=0; i < lblMatrix.numElem(); i++)
            dPtr[i] = rand() % 3;

        mPluginServices->updateDisplay();
    }

    void showMsgBoxClicked()
    {
        QMessageBox::information( mPluginServices->getMainWindow(), "Clicked me!", "You have just clicked me." );
    }
};

#endif // TESTPLUGIN_H
