#ifndef FIJIHELPER_H
#define FIJIHELPER_H

#include <QString>
#include <QProcess>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>

#include "Matrix3D.h"

struct FijiConfig
{
    QString fijiExe;
};

class FijiShow3D
{
public:
    FijiShow3D() {}

    void setConfig( const FijiConfig& c ) {
        mConfig = c;
    }

    template<typename T>
    bool run( const Matrix3D<T> &data )
    {
        QString fVolume = QDir::tempPath() + "/annVol.tif";
        QString fScript = QDir::tempPath() + "/annVol.ijm";

        bool ret = data.save( fVolume.toLatin1().constData() );
        if (!ret)
            return false;

        QFile script( fScript );
        if (!script.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

        // open volume
        script.write( QString("open(\"%1\");").arg(fVolume).toLatin1() );

        // run 3D viewer
        script.write( QString("run(\"3D Viewer\");").toLatin1() );

        // opts
        script.write( QString(
                          "call(\"ij3d.ImageJ3DViewer.setCoordinateSystem\", \"false\");"
                          "call(\"ij3d.ImageJ3DViewer.add\", \"annVol.tif\", \"None\", \"annVol.tif\", \"0\", \"true\", \"true\", \"true\", \"1\", \"0\");"
                          ).toLatin1() );

        // close img window
        script.write( QString("selectWindow(\"annVol.tif\");").toLatin1() );
        script.write( QString("close();").toLatin1() );

        script.close();

        QString toRun = mConfig.fijiExe + " -macro " + fScript + " &";
        system( toRun.toLatin1().constData() );

        return true;
    }

private:
    FijiConfig mConfig;
};

#endif // FIJIHELPER_H
