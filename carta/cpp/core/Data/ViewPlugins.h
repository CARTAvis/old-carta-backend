/***
 * Manages the data for a view displaying the plugins that have been loaded.
 *
 */

#pragma once

#include "State/ObjectManager.h"

namespace Carta {

namespace Data {

class ViewPlugins : public Carta::State::CartaObject {

public:

    virtual ~ViewPlugins();
    const static QString CLASS_NAME;

private:

    static bool m_registered;
    ViewPlugins( const QString& path, const QString& id );

    class Factory;

    static const QString PLUGINS;
    static const QString DESCRIPTION;
    static const QString VERSION;
    static const QString ERRORS;
    static const QString STAMP;
    ViewPlugins( const ViewPlugins& other);
    ViewPlugins& operator=( const ViewPlugins& other );
};
}
}
