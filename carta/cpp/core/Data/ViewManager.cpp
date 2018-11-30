#include "Data/ViewManager.h"
#include "Data/Image/Controller.h"
#include "Data/DataLoader.h"
#include "Data/Error/ErrorManager.h"
#include "Data/ViewPlugins.h"
#include "Data/Util.h"
#include "State/UtilState.h"
#include <QTime>
#include <QDebug>
#include <QElapsedTimer>

namespace Carta {
namespace Data {

class ViewManager::Factory : public Carta::State::CartaObjectFactory {

public:
    Factory():
        CartaObjectFactory( "ViewManager" ){}
    Carta::State::CartaObject * create (const QString & path, const QString & id)
    {
        return new ViewManager (path, id);
    }
};

const QString ViewManager::CLASS_NAME = "ViewManager";
const QString ViewManager::SOURCE_ID = "sourceId";
const QString ViewManager::SOURCE_PLUGIN = "sourcePlugin";
const QString ViewManager::SOURCE_LOCATION_ID = "sourceLocateId";
const QString ViewManager::DEST_ID = "destId";
const QString ViewManager::DEST_PLUGIN = "destPlugin";
const QString ViewManager::DEST_LOCATION_ID = "destLocateId";

bool ViewManager::m_registered =
        Carta::State::ObjectManager::objectManager()->registerClass ( CLASS_NAME,
                                                   new ViewManager::Factory());

ViewManager::ViewManager( const QString& path, const QString& id)
    : CartaObject( CLASS_NAME, path, id ),
      m_pluginsLoaded( nullptr ) {
}

void ViewManager::_clearControllers( int startIndex, int upperBound ){
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    for ( int i = upperBound-1; i >= startIndex; i-- ){
        objMan->destroyObject( m_controllers[i]->getId() );
        m_controllers.removeAt(i);
    }
}

QString ViewManager::getObjectId( const QString& plugin, int index, bool forceCreate ){
    QString viewId("");
    if ( plugin == Controller::PLUGIN_NAME ){
        if ( 0 <= index && index < m_controllers.size() && !forceCreate) {
            viewId = m_controllers[index]->getPath();
        }
        else {
            if ( index == -1 ){
                index = m_controllers.size();
            }
            viewId = _makeController(index);
        }
    }
    else if ( plugin == ViewPlugins::CLASS_NAME ){
        viewId = _makePluginList();
    }
    else {
        qDebug() << "Unrecognized top level window type: "<<plugin;
    }
    return viewId;
}

int ViewManager::getControllerCount() const {
    int controllerCount = m_controllers.size();
    return controllerCount;
}

QString ViewManager::registerView(const QString & params) {
    const QString PLUGIN_ID("pluginId");
    const QString INDEX("index");
    std::set<QString> keys = {PLUGIN_ID, INDEX};
    std::map<QString,QString> dataValues = Carta::State::UtilState::parseParamMap(params, keys);
    bool validIndex = false;
    int index = dataValues[INDEX].toInt(&validIndex);
    QString viewId( "");
    if ( validIndex ){
        viewId = getObjectId(dataValues[PLUGIN_ID], index);
    }
    else {
        qWarning() << "Register view: invalid index: " + dataValues[PLUGIN_ID];
    }
    return viewId;
}

QString ViewManager::_makeController(int index) {
    int currentCount = m_controllers.size();
    CARTA_ASSERT(0 <= index && index <= currentCount);
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    Controller* controlObj = objMan->createObject<Controller>();
    m_controllers.insert(index, controlObj);
    for (int i = index; i < currentCount + 1; i++) {
        m_controllers[i]->setIndex(i);
    }
    QString path = m_controllers[index]->getPath();
    return path;
}

QString ViewManager::_makePluginList(){
    if ( !m_pluginsLoaded ){
        //Initialize a view showing the plugins that have been loaded
        Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
        m_pluginsLoaded = objMan->createObject<ViewPlugins>();
    }
    QString pluginsPath = m_pluginsLoaded->getPath();
    return pluginsPath;
}

ViewManager::~ViewManager() {
    delete m_pluginsLoaded;
    _clearControllers(0, m_controllers.size());

    //Delete the statics
    CartaObject* obj =  Util::findSingletonObject<ErrorManager>();
    delete obj;

    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    objMan->printObjects();
}
}
}
