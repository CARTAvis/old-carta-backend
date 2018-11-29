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
      m_dataLoader( nullptr ),
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
        if ( 0 <= index && index < m_controllers.size()&&!forceCreate){
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

QString ViewManager::dataLoaded(const QString & params) {

    const QString DATA( "data");
    std::set<QString> keys = {Util::ID,DATA};
    std::map<QString,QString> dataValues = Carta::State::UtilState::parseParamMap( params, keys );
    bool fileLoaded = false;

    // get the loading file name
    QString fileName = dataValues[DATA].split("/").last();

    // get the timer for the function "loadFile(...)"
    QElapsedTimer timer;
    timer.start();

    if (CARTA_RUNTIME_CHECKS) {
        qCritical() << "<> Start loading" << fileName << "!";
    }

    // execute the function "loadFile(...)"
    QString result = loadFile( dataValues[Util::ID], dataValues[DATA],&fileLoaded);

    // set the output stype of log lines
    int elapsedTime = timer.elapsed();
    if (CARTA_RUNTIME_CHECKS) {
        qCritical() << "<> Total loading time for" << fileName << ":" << elapsedTime << "ms";
    }

    if ( !fileLoaded ){
        Util::commandPostProcess( result);
    }
    return "";

}

QString ViewManager::registerView(const QString & params){

    const QString PLUGIN_ID( "pluginId");
    const QString INDEX( "index");
    std::set<QString> keys = {PLUGIN_ID, INDEX};
    std::map<QString,QString> dataValues = Carta::State::UtilState::parseParamMap( params, keys );
    bool validIndex = false;
    int index = dataValues[INDEX].toInt(&validIndex);
    QString viewId( "");
    if ( validIndex ){
        viewId = getObjectId( dataValues[PLUGIN_ID], index );
    }
    else {
        qWarning()<< "Register view: invalid index: "+dataValues[PLUGIN_ID];
    }
    return viewId;
}

QString ViewManager::loadFile( const QString& controlId, const QString& fileName, bool* fileLoaded){
    QString result;
    int controlCount = getControllerCount();
    for ( int i = 0; i < controlCount; i++ ){
        const QString controlPath= m_controllers[i]->getPath();
        if ( controlId  == controlPath ){
           //Add the data to it
            _makeDataLoader();
           QString path = m_dataLoader->getFile( fileName, "" );
//           result = m_controllers[i]->addData( path, fileLoaded );
           break;
        }
    }
    return result;
}

QString ViewManager::_makeController( int index ){
    int currentCount = m_controllers.size();
    CARTA_ASSERT( 0 <= index && index <= currentCount );
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    Controller* controlObj = objMan->createObject<Controller>();
    m_controllers.insert( index, controlObj );
    for ( int i = index; i < currentCount + 1; i++ ){
        m_controllers[i]->setIndex( i );
    }
    QString path = m_controllers[index]->getPath();
    return path;
}

void ViewManager::_makeDataLoader(){
    if ( m_dataLoader == nullptr ){
        Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
        m_dataLoader = objMan->createObject<DataLoader>();
        //m_dataLoader = Util::findSingletonObject<DataLoader>();
    }
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

void ViewManager::_pluginsChanged( const QStringList& names, const QStringList& oldNames ){
    QMap<QString, int> pluginMap;
    QMap<QString, QVector<int>> insertionIndices;
    int oldNameCount = oldNames.size();
    for ( int i = 0; i < names.size(); i++ ){
        if ( !insertionIndices.contains( names[i]) ){
            insertionIndices[names[i]] = QVector<int>();
        }
        //Store the accumulated count for this plugin.
        if ( pluginMap.contains( names[i]) ){
            pluginMap[names[i]] = pluginMap[names[i]]+1;
        }
        else {
            pluginMap[names[i]] = 1;
        }

        //If there is an existing plugin of this type at i,
        //then we don't need to do an insertion, otherwise, we may need to
        //if the new plugin count ends up being more than the old one.
        if ( i >= oldNameCount || oldNames[i] != names[i] ){
            QVector<int> & indices = insertionIndices[names[i]];
            indices.append( pluginMap[names[i]] - 1);
        }
    }

    //Add counts of zero for old ones that no longer exist.
    for ( int i = 0; i < oldNameCount; i++ ){
        if ( !names.contains( oldNames[i] ) ){
            pluginMap[oldNames[i]] = 0;
            insertionIndices[oldNames[i]] = QVector<int>(0);
        }
    }
}

ViewManager::~ViewManager(){
    delete m_dataLoader;
    delete m_pluginsLoaded;
    _clearControllers( 0, m_controllers.size() );

    //Delete the statics
    CartaObject* obj =  Util::findSingletonObject<ErrorManager>();
    delete obj;

    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    objMan->printObjects();
}
}
}
