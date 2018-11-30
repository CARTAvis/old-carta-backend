/***
 * Main class that manages the data state for the views.
 *
 */

#pragma once

#include "State/ObjectManager.h"
#include <QVector>
#include <QObject>

namespace Carta {
namespace Data {

class Controller;
class ViewPlugins;

class ViewManager : public QObject, public Carta::State::CartaObject {

    Q_OBJECT

public:
    /**
     * Return the unique server side id of the object with the given name and index in the
     * layout.
     * @param pluginName an identifier for the kind of object.
     * @param index an index in the case where there is more than one object of the given kind
     *      in the layout.
     */
    QString getObjectId( const QString& pluginName, int index, bool forceCreate = false );

    /**
     * Return the number of controllers (image views).
     * @return - the controller count.
     */
    int getControllerCount() const;

    /**
     * Return the number of colormap views.
     * @return - the colormap count.
     */
    int getColormapCount() const;

    static const QString CLASS_NAME;

    /**
     * Destructor.
     */
    virtual ~ViewManager();

    QString registerView(const QString & params);

private slots:

private:
    ViewManager( const QString& path, const QString& id);

    class Factory;

    void _clearControllers( int startIndex, int upperBound );

    QString _makePluginList();

    QString _makeController( int index );

    //A list of Controllers requested by the client.
    QList <Controller* > m_controllers;

    static bool m_registered;
    ViewPlugins* m_pluginsLoaded;

    const static QString SOURCE_ID;
    const static QString SOURCE_PLUGIN;
    const static QString SOURCE_LOCATION_ID;
    const static QString DEST_ID;
    const static QString DEST_PLUGIN;
    const static QString DEST_LOCATION_ID;

    ViewManager( const ViewManager& other);
    ViewManager& operator=( const ViewManager& other );
};

}
}
