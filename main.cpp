#include <QtGui/QApplication>
#include "annotatorwnd.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AnnotatorWnd w;
    w.show();

    return a.exec();
}
