#include <QtGui/QApplication>
#include "annotatorwnd.h"

#include <cstdio>
#include <cstdlib>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // fix for QT menu icons in ubuntu/linux
#ifdef Q_WS_X11
    qDebug() << "Calling gconftool";
    system("gconftool-2 --type Boolean --set /desktop/gnome/interface/menus_have_icons True");
#endif

    AnnotatorWnd w;
    w.show();

    return a.exec();
}
