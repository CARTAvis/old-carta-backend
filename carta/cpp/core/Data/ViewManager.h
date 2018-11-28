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
class DataLoader;
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

    /**
     * Load the file into the controller with the given id.
     * @param fileName a locater for the data to load.
     * @param objectId the unique server side id of the controller which is
     * responsible for displaying the file.
     * @param success - set to true if the file was successfully loaded; false otherwise.
     * @return - the identifier of the server-side object managing the image if the file
     *      was successfully loaded; otherwise, an error message.
     */
    QString loadFile( const QString& objectId, const QString& fileName, bool* success);

    static const QString CLASS_NAME;

    /**
     * Destructor.
     */
    virtual ~ViewManager();

    QString dataLoaded(const QString & params);
    QString registerView(const QString & params);

private slots:
    void _pluginsChanged( const QStringList& names, const QStringList& oldNames );

private:
    ViewManager( const QString& path, const QString& id);

    class Factory;

    void _clearControllers( int startIndex, int upperBound );

    QString _makePluginList();

    QString _makeController( int index );

    void _makeDataLoader();

    //A list of Controllers requested by the client.
    QList <Controller* > m_controllers;

    static bool m_registered;
    DataLoader* m_dataLoader;
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
