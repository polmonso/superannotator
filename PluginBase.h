#ifndef PLUGINBASE_H
#define PLUGINBASE_H

/**
 * This is a base class for plugins
 */

#include <Matrix3D.h>
#include <QObject>
#include <QMenu>
#include <QList>
#include "CommonTypes.h"

#include "PluginServices.h"

class AnnotatorWnd;

class PluginBase : public QObject
{
    Q_OBJECT

public:
    explicit PluginBase( QObject *parent = 0 ) : QObject(parent) {}
    virtual bool    initializePlugin( const PluginServices & ) = 0;

    // must return the plugin's name
    virtual QString pluginName() = 0;
};


#endif // PLUGINBASE_H