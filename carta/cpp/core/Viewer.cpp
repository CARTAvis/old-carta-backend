#include "CartaLib/CartaLib.h"
#include "CartaLib/Hooks/Initialize.h"
#include "Viewer.h"
#include "Globals.h"
#include "IPlatform.h"
#include "State/ObjectManager.h"
#include "Data/ViewManager.h"
#include "Data/Image/Controller.h"
#include "PluginManager.h"
#include "MainConfig.h"
#include "MyQApp.h"
#include "CmdLine.h"
#include "CartaLib/Hooks/GetPersistentCache.h"

#include <QImage>
#include <QColor>
#include <QPainter>
#include <QDebug>
#include <QCache>
#include <QCoreApplication>
#include <QJsonObject>
#include <QDir>
#include <QJsonArray>

#include <cmath>
#include <iostream>
#include <limits>

#include <QThread>

Viewer::Viewer() :
    QObject( nullptr ),
    m_viewManager( nullptr)
{
    int port = Globals::instance()->cmdLineInfo()-> scriptPort();
    qDebug() << "Port="<<port;
    if ( port < 0 ) {
        qDebug() << "Not listening to scripted commands.";
    }
    else {
        qDebug() << "Listening to scripted commands on port " << port;
    }
    m_devView = false;
}

void
Viewer::start()
{
    auto & globals = * Globals::instance();

    QString name = QThread::currentThread()->objectName();
    qDebug() << "Viewer::start() name of the current thread:" << name;

	if ( m_viewManager == nullptr ){
	    Carta::State::ObjectManager* objectManager = Carta::State::ObjectManager::objectManager();
        Carta::Data::ViewManager* vm = objectManager->createObject<Carta::Data::ViewManager> ();
        m_viewManager.reset( vm );
	}

    qDebug() << "Viewer has been initialized.";
}

void Viewer::DBClose() {

    std::shared_ptr<Carta::Lib::IPCache> m_diskCache;

    // find the unique shared_ptr of cache and release it by force
    auto res = Globals::instance()-> pluginManager()
               -> prepare < Carta::Lib::Hooks::GetPersistentCache > ().first();
    if ( res.isNull() || ! res.val() ) {
        qWarning( "Could not find a disk cache plugin." );
        m_diskCache = nullptr;
    }
    else {
        m_diskCache = res.val();
        m_diskCache->Release();
    }
}

void Viewer::setDeveloperView( ){
    m_devView = true;
}
