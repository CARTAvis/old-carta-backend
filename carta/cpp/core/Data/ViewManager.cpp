#include "Data/ViewManager.h"
//#include "Data/Colormap/Colormap.h"
//#include "Data/Colormap/Colormaps.h"
#include "Data/Image/Controller.h"
#include "Data/Image/CoordinateSystems.h"
//#include "Data/Image/Grid/Themes.h"
//#include "Data/Image/Grid/Fonts.h"
//#include "Data/Image/Grid/LabelFormats.h"
//#include "Data/Image/Contour/ContourGenerateModes.h"
//#include "Data/Image/Contour/ContourTypes.h"
//#include "Data/Image/Contour/ContourSpacingModes.h"
//#include "Data/Image/Contour/ContourStyles.h"
#include "Data/Image/LayerCompositionModes.h"
#include "Data/DataLoader.h"
//#include "Data/Colormap/Gamma.h"
//#include "Data/Colormap/TransformsData.h"
//#include "Data/Colormap/TransformsImage.h"
#include "Data/Error/ErrorManager.h"
#include "Data/ILinkable.h"
#include "Data/Region/RegionTypes.h"
#include "Data/Statistics/Statistics.h"
#include "Data/ViewPlugins.h"
#include "Data/Units/UnitsFrequency.h"
#include "Data/Units/UnitsIntensity.h"
#include "Data/Units/UnitsSpectral.h"
#include "Data/Units/UnitsWavelength.h"
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
//        for ( Colormap* map : m_colormaps ){
//            map->removeLink( m_controllers[i]);
//        }
        for ( Statistics* stat : m_statistics ){
            stat->removeLink( m_controllers[i]);
        }
        objMan->destroyObject( m_controllers[i]->getId() );
        m_controllers.removeAt(i);
    }
}

void ViewManager::_clearColormaps( int startIndex, int upperBound ){
//    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
//    for ( int i = upperBound-1; i >= startIndex; i-- ){
//        objMan->destroyObject( m_colormaps[i]->getId() );
//        m_colormaps.removeAt( i );
//    }
}

void ViewManager::_clearStatistics( int startIndex, int upperBound ){
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    for ( int i = upperBound-1; i >= startIndex; i-- ){
        objMan->destroyObject( m_statistics[i]->getId() );
        m_statistics.removeAt( i );
    }
}


int ViewManager::_findListIndex( const QString& sourcePlugin, int pluginIndex, const QStringList& plugins ) const{
    int pluginCount = -1;
    int listIndex = -1;
    for ( int i = 0; i < plugins.size(); i++ ){
        if ( plugins[i] == sourcePlugin ){
            pluginCount++;
        }
        if ( pluginCount == pluginIndex ){
            listIndex = i;
            break;
        }
    }
    return listIndex;
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
    /*else if ( plugin == Colormap::CLASS_NAME ){
        if ( 0 <= index && index < m_colormaps.size() && !forceCreate){
            viewId = m_colormaps[index]->getPath();
        }
        else {
            if ( index == -1 ){
                index = m_colormaps.size();
            }
            viewId = _makeColorMap( index );
        }
    }*/
    else if ( plugin == Statistics::CLASS_NAME ){
        if ( 0 <= index && index < m_statistics.size() && !forceCreate){
            viewId = m_statistics[index]->getPath();
        }
        else {
            if ( index == -1 ){
                index = m_statistics.size();
            }
            viewId = _makeStatistics( index );
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

//int ViewManager::getColormapCount() const {
//    int colorMapCount = m_colormaps.size();
//    return colorMapCount;
//}

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

void ViewManager::_initCallbacks(){

    //Callback for registering a view.
    addCommandCallback( "registerView", [=] (const QString & /*cmd*/,
            const QString & params, const QString & /*sessionId*/) -> QString {
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
    });

    //Callback for linking a window with another window
    addCommandCallback( "linkAdd", [=] (const QString & /*cmd*/,
            const QString & params, const QString & /*sessionId*/) -> QString {
        std::set<QString> keys = {SOURCE_ID, DEST_ID};
        std::map<QString,QString> dataValues = Carta::State::UtilState::parseParamMap( params, keys );
        QString result = linkAdd( dataValues[SOURCE_ID], dataValues[DEST_ID]);
        Util::commandPostProcess( result);
        return result;
    });

    //Callback for linking a window with another window.
    addCommandCallback( "linkRemove", [=] (const QString & /*cmd*/,
            const QString & params, const QString & /*sessionId*/) -> QString {
        std::set<QString> keys = {SOURCE_ID, DEST_ID};
        std::map<QString,QString> dataValues = Carta::State::UtilState::parseParamMap( params, keys );
        QString result = linkRemove( dataValues[SOURCE_ID], dataValues[DEST_ID]);
        Util::commandPostProcess( result );
        return result;
    });
}

QString ViewManager::_isDuplicateLink( const QString& sourceName, const QString& destId ) const {
    QString result;
    bool alreadyLinked = false;
//    if ( sourceName == Colormap::CLASS_NAME ){
//        int colorCount = m_colormaps.size();
//        for ( int i = 0; i < colorCount; i++  ){
//            alreadyLinked = m_colormaps[i]->isLinked( destId );
//            if ( alreadyLinked ){
//                break;
//            }
//        }
//    }

    if ( alreadyLinked ){
        result = "Destination can only be linked to one "+sourceName;
    }
    return result;
}


QString ViewManager::linkAdd( const QString& sourceId, const QString& destId ){
    QString result;
    Carta::State::ObjectManager* objManager = Carta::State::ObjectManager::objectManager();
    QString dId = objManager->parseId( destId );
    Carta::State::CartaObject* destObj = objManager->getObject( dId );
    if ( destObj != nullptr ){
        QString id = objManager->parseId( sourceId );
        Carta::State::CartaObject* sourceObj = objManager->getObject( id );
        ILinkable* linkSource = dynamic_cast<ILinkable*>( sourceObj );
        if ( linkSource != nullptr ){
            if ( !linkSource->isLinked( destId )){
                result = _isDuplicateLink(sourceObj->getType(), destId );
                if ( result.isEmpty() ){
                    result = linkSource->addLink( destObj );
                }
            }
        }
        else {
            result = "Unrecognized add link source: "+sourceId;
        }
    }
    else {
        result = "Unrecognized add link destination: "+dId;
    }
    return result;
}

QString ViewManager::linkRemove( const QString& sourceId, const QString& destId ){
    QString result;
    Carta::State::ObjectManager* objManager = Carta::State::ObjectManager::objectManager();
    QString dId = objManager->parseId( destId );
    Carta::State::CartaObject* destObj = objManager->getObject( dId );
    if ( destObj != nullptr ){
        QString id = objManager->parseId( sourceId );
        Carta::State::CartaObject* sourceObj = objManager->getObject( id );
        ILinkable* linkSource = dynamic_cast<ILinkable*>( sourceObj );
        if ( linkSource != nullptr ){
            result = linkSource->removeLink( destObj );
        }
        else {
            result = "Could not remove link, unrecognized source: "+sourceId;
        }
    }
    else {
        result = "Could not remove link, unrecognized destination: "+destId;
    }
    return result;
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


void ViewManager::_moveView( const QString& plugin, int oldIndex, int newIndex ){
    if ( oldIndex != newIndex && oldIndex >= 0 && newIndex >= 0 ){
        if ( plugin == Controller::PLUGIN_NAME ){
            int controlCount = m_controllers.size();
            if ( oldIndex < controlCount && newIndex < controlCount ){
                Controller* controller = m_controllers[oldIndex];
                m_controllers.removeAt(oldIndex );
                m_controllers.insert( newIndex, controller );
            }
        }
//        else if ( plugin == Colormap::CLASS_NAME ){
//            int colorCount = m_colormaps.size();
//            if ( oldIndex < colorCount && newIndex < colorCount ){
//                Colormap* colormap = m_colormaps[oldIndex];
//                m_colormaps.removeAt(oldIndex );
//                m_colormaps.insert( newIndex, colormap );
//            }
//        }
    }
    else {
        qWarning() << "Move view insert indices don't make sense "<<oldIndex<<" and "<<newIndex;
    }
}

/*QString ViewManager::_makeColorMap( int index ){
    int currentCount = m_colormaps.size();
    CARTA_ASSERT( 0 <= index && index <= currentCount );
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    Colormap* colormap = objMan->createObject<Colormap>();
    m_colormaps.insert( index, colormap );
    for ( int i = index; i < currentCount + 1; i++ ){
        m_colormaps[i]->setIndex( i );
    }
    QString path = m_colormaps[index]->getPath();
   return path;
}*/

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

QString ViewManager::_makeStatistics( int index ){
    int currentCount = m_statistics.size();
    CARTA_ASSERT( 0 <= index && index <= currentCount );
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    Statistics* statObj = objMan->createObject<Statistics>();
    m_statistics.insert( index, statObj );
    for ( int i = index; i < currentCount + 1; i++ ){
        m_statistics[i]->setIndex( i );
    }
    //To try to establish reasonable defaults, if there is a single statistics display
    //and a single controller display, assume the user wants them linked.
    if ( m_statistics.size() == 1 && m_controllers.size() == 1 ){
        m_statistics[0]->addLink( m_controllers[0] );
    }
    return m_statistics[index]->getPath();
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
//    _clearColormaps( 0, m_colormaps.size() );
    _clearStatistics( 0, m_statistics.size() );
    _clearControllers( 0, m_controllers.size() );

    //Delete the statics
//    CartaObject* obj = Util::findSingletonObject<Colormaps>();
//    delete obj;
//    CartaObject* obj =  Util::findSingletonObject<TransformsData>();
//    delete obj;
//    obj =  Util::findSingletonObject<TransformsImage>();
//    delete obj;
//    CartaObject* obj = Util::findSingletonObject<Gamma>();
//    delete obj;
    CartaObject* obj =  Util::findSingletonObject<ErrorManager>();
    delete obj;
//    obj =  Util::findSingletonObject<LabelFormats>();
//    delete obj;
    obj =  Util::findSingletonObject<CoordinateSystems>();
    delete obj;
//    obj =  Util::findSingletonObject<Themes>();
//    delete obj;
//    obj =  Util::findSingletonObject<ContourGenerateModes>();
//    delete obj;
//    obj =  Util::findSingletonObject<ContourTypes>();
//    delete obj;
//    obj =  Util::findSingletonObject<ContourSpacingModes>();
//    delete obj;
//    obj =  Util::findSingletonObject<ContourStyles>();
//    delete obj;
    obj =  Util::findSingletonObject<LayerCompositionModes>();
    delete obj;
    obj = Util::findSingletonObject<RegionTypes>();
    delete obj;
    obj =  Util::findSingletonObject<UnitsFrequency>();
    delete obj;
    obj =  Util::findSingletonObject<UnitsSpectral>();
    delete obj;
    obj =  Util::findSingletonObject<UnitsWavelength>();
    delete obj;
//    obj = Util::findSingletonObject<Fonts>();
//    delete obj;

    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    objMan->printObjects();
}
}
}
