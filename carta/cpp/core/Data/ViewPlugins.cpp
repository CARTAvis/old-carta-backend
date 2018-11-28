#include "ViewPlugins.h"
#include "Globals.h"
#include "PluginManager.h"
#include "Image/Controller.h"
#include "Util.h"
#include "State/UtilState.h"

#include <QDir>
#include <QDebug>

namespace Carta {
namespace Data {

using Carta::State::UtilState;

class ViewPlugins::Factory : public Carta::State::CartaObjectFactory {

public:

    Factory():
        CartaObjectFactory( PLUGINS ){};

    Carta::State::CartaObject * create (const QString & path, const QString & id)
    {
        return new ViewPlugins (path, id);
    }
};

const QString ViewPlugins::PLUGINS = "pluginList";
const QString ViewPlugins::DESCRIPTION = "description";
const QString ViewPlugins::VERSION = "version";
const QString ViewPlugins::ERRORS = "loadErrors";
const QString ViewPlugins::STAMP = "pluginCount";
const QString ViewPlugins::CLASS_NAME = "ViewPlugins";

bool ViewPlugins::m_registered =
        Carta::State::ObjectManager::objectManager()->registerClass ( CLASS_NAME,
                                                   new ViewPlugins::Factory());

ViewPlugins::ViewPlugins( const QString& path, const QString& id):
    CartaObject( CLASS_NAME, path, id ){
    _initializeDefaultState();
}

void ViewPlugins::_insertPlugin( int ind, const QString& name, const QString& description,
        const QString& type, const QString& version, const QString& errors ){
    QString index = QString("%1").arg(ind);
    QString arrayIndex = UtilState::getLookup(PLUGINS, index);
}

void ViewPlugins::_initializeDefaultState(){
    int ind = 0;
    _insertPlugin( ind, Controller::PLUGIN_NAME, "Image Display", "", "", "");
    ind++;
}

ViewPlugins::~ViewPlugins(){

}
}
}
