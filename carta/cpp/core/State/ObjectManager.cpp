
 /* CartaObjectManager.cc
 *
 *  Created on: Oct 3, 2014
 *      Author: jjacobs
 */

#include "ObjectManager.h"
#include "Globals.h"
#include "UtilState.h"
#include <QDebug>
#include <cassert>
#include <set>
#include <QThread>

using namespace std;

namespace Carta {

namespace State {

QList<QString> CartaObjectFactory::globalIds = {"ChannelUnits", "ErrorManager", "GenerateModes"};

//grimmer:
//  become non-singleton:Layout", "ViewManager", "DataLoader"
// internal change data(dangerous): UnitsIntensity
//  callback(not compatible with new arch): PlotStyles, x DataLoader, layout (setlayoutsize)?
//     CoordinateSystems, ErrorManager, Preferences, PreferencesSave, Themes

QString CartaObject::addIdToCommand (const QString & command) const {
    QString fullCommand = m_path;
    if ( command.size() > 0 ){
        fullCommand = fullCommand + CommandDelimiter + command;
    }
    return fullCommand;
}

QString CartaObject::getStateString( const QString& /*sessionId*/, SnapshotType /*type*/ ) const {
    return "";
}

QString
CartaObject::getClassName () const
{
    return m_className;
}

QString
CartaObject::getId () const
{
    return m_id;
}

QString
CartaObject::getPath () const
{
    return m_path;
}

CartaObject::CartaObject (const QString & className,
                          const QString & path,
                          const QString & id)
:
  m_className (className),
  m_id (id),
  m_path (path) {
}

QString CartaObject::getSnapType(CartaObject::SnapshotType /*snapType*/) const {
    return m_className;
}

QString CartaObject::getType() const {
    return m_className;
}

void CartaObject::setIndex( int index ){
    CARTA_ASSERT( index >= 0 );
    if (m_index != index) {
        m_index = index;
    }
}

void CartaObject::resetStateData( const QString& /*state*/ ){
}

void
CartaObject::addCommandCallback (const QString & rawCommand, IConnector::CommandCallback callback)
{
    conn()-> addCommandCallback ( addIdToCommand( rawCommand ), callback);
}

void
CartaObject::addMessageCallback (const QString & messageType, IConnector::MessageCallback callback)
{
    // Try to use another way to register callback
    // QString sessionID = Globals::instance()->sessionID();
    // conn()-> addMessageCallback ( sessionID + ":" + messageType, callback);
    conn()-> addMessageCallback ( addIdToCommand(messageType), callback);
    // use for test, without assign the id of non-global objects
    // conn() -> addMessageCallback( messageType, callback);
}

QString
CartaObject::removeId (const QString & commandAndId)
{
    // Command should have the ID as a prefix.
    CARTA_ASSERT(commandAndId.count (CommandDelimiter) == 1);

    // return command without the ID prefix
    return commandAndId.section (CommandDelimiter, 1);
}

IConnector *
CartaObject::conn() {
    IConnector * conn = Globals::instance()-> connector();
    return conn;

}

CartaObject::~CartaObject () {
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    objMan->removeObject( getId() );
};

const QString ObjectManager::CreateObject = "CreateObject";
const QString ObjectManager::ClassName = "ClassName";
const QString ObjectManager::DestroyObject = "DestroyObject";

const QString ObjectManager::STATE_ARRAY = "states";
const QString ObjectManager::STATE_ID = "id";
const QString ObjectManager::STATE_VALUE = "state";

ObjectManager::ObjectManager ()
:       m_root( "CartaObjects"),
        m_sep( "/"),
    m_nextId (0){

}

QString ObjectManager::getRootPath() const {
    return  m_sep + m_root;
}

QString ObjectManager::getRoot() const {
    return m_root;
}

QString ObjectManager::parseId( const QString& path ) const {
    QString basePath = m_sep + m_root + m_sep;
    int rootIndex = path.indexOf( basePath );
    QString id;
    if ( rootIndex >= 0 ){
        id = path.right( path.length() - basePath.length());
    }
    return id;
}

void ObjectManager::printObjects(){
    for(map<QString,ObjectRegistryEntry>::iterator it = m_objects.begin(); it != m_objects.end(); ++it) {
        QString firstId = it->first;
        QString classId = it->second.getClassName();
        qDebug() << "id="<<firstId<<" class="<<classId;
    }
}

CartaObject* ObjectManager::removeObject( const QString& id ){
    CartaObject * object = getObject (id);
    if ( object ){
        m_objects.erase (id);
    }
    return object;
}

QString
ObjectManager::destroyObject (const QString & id)
{
    CartaObject* object = removeObject( id );
    delete object;
    return "";
}

CartaObject *
ObjectManager::getObject (const QString & id)
{

    ObjectRegistry::iterator i = m_objects.find (id);

    CartaObject * result = 0;

    if (i != m_objects.end()){
        result = i->second.getObject();
    }

    return result;
}

void
ObjectManager::initialize()
{

    // Register command handler(s) for create, etc.

    IConnector * connector = Globals::instance()->connector();
    connector->addCommandCallback (QString (CreateObject), OnCreateObject (this));
    connector->addCommandCallback (QString (DestroyObject), OnDestroyObject (this));

}

ObjectManager *
ObjectManager::objectManager ()
{
    // Implements a singleton pattern
    static ObjectManager om;
    return &om;
}

ObjectManager::~ObjectManager (){

}

bool
ObjectManager::registerClass (const QString & className, CartaObjectFactory * factory)
{
    assert (m_classes.find (className) == m_classes.end());

    m_classes [className] = ClassRegistryEntry (className, factory);

    return true; // failure gets "handled" by the assert for now
}

bool ExampleCartaObject::m_registered =
    ObjectManager::objectManager()->registerClass ("edu.nrao.carta.ExampleCartaObject",
                                                   new ExampleCartaObject::Factory());

const QString ExampleCartaObject::DoSomething ("DoSomething");

ExampleCartaObject::ExampleCartaObject (const QString & path, const QString & id)
: CartaObject (path, id, "edu.nrao.carta.ExampleCartaObject")
{
    //addCommandCallback (DoSomething, OnCommand (this, & ExampleCartaObject::doSomething));
    //addCommandCallback (DoSomething, wrapCommandHandler (this, & ExampleCartaObject::doSomething));
}

}
}
