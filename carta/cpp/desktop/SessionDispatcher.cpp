/**
 *
 **/

#include "SessionDispatcher.h"
#include "CartaLib/LinearMap.h"
#include "core/MyQApp.h"
#include "core/SimpleRemoteVGView.h"
#include <iostream>
#include <QImage>
#include <QPainter>
#include <QXmlInputSource>
#include <cmath>
#include <QTime>
#include <QTimer>
#include <QCoreApplication>
#include <functional>

#include <QStringList>

#include <thread>
// #include "websocketclientwrapper.h"
// #include "websockettransport.h"
// #include "qwebchannel.h"
#include <QBuffer>
#include <QThread>

#include "NewServerConnector.h"

#include "CartaLib/Proto/register_viewer.pb.h"
#include "CartaLib/Proto/set_image_channels.pb.h"
#include "CartaLib/Proto/set_image_view.pb.h"

#include "Globals.h"
#include "core/CmdLine.h"

void SessionDispatcher::startWebSocket(){

    int port = Globals::instance()->cmdLineInfo()-> port();

    if ( port < 0 ) {
        port = 3002;
        qDebug() << "Using default SessionDispatcher port" << port;
       }
    else {
        qDebug() << "SessionDispatcher listening on port" << port;
    }

    if (!m_hub.listen(port)){
        qFatal("Failed to open web socket server.");
        return;
    }

    m_hub.onConnection([this](uWS::WebSocket<uWS::SERVER> *ws, uWS::HttpRequest req){
        onNewConnection(ws);
    });

    m_hub.onMessage([this](uWS::WebSocket<uWS::SERVER> *ws, char* message, size_t length, uWS::OpCode opcode){
        if (opcode == uWS::OpCode::TEXT){
            onTextMessage(ws, message, length);
        }
        else if (opcode == uWS::OpCode::BINARY){
            onBinaryMessage(ws, message, length);
        }
    });

    // repeat calling non-blocking poll() in qt event loop
    loopPoll();
}

void SessionDispatcher::loopPoll(){
    m_hub.poll();
    // submit a queue into qt eventloop
    defer([this](){
        loopPoll();
    });
}

SessionDispatcher::SessionDispatcher() {

}

SessionDispatcher::~SessionDispatcher() {

}

void SessionDispatcher::onNewConnection(uWS::WebSocket<uWS::SERVER> *socket) {
    qDebug() << "A new connection!!";
}

void SessionDispatcher::onTextMessage(uWS::WebSocket<uWS::SERVER> *ws, char* message, size_t length) {
    NewServerConnector* connector= sessionList[ws];
    QString cmd = QString::fromStdString(std::string(message, length));
    if (connector != nullptr){
        emit connector->onTextMessageSignal(cmd);
    }
}

void SessionDispatcher::onBinaryMessage(uWS::WebSocket<uWS::SERVER> *ws, char* message, size_t length) {

    if (length < EVENT_NAME_LENGTH + EVENT_ID_LENGTH) {
        qFatal("[SessionDispatcher] Illegal message.");
        return;
    }

    // get the message Name
    size_t nullIndex = 0;
    for (size_t i = 0; i < EVENT_NAME_LENGTH; i++) {
        if (!message[i]) {
            nullIndex = i;
            break;
        }
    }
    QString eventName = QString::fromStdString(std::string(message, nullIndex));

    // get the message Id
    uint32_t eventId = *((uint32_t*) (message + EVENT_NAME_LENGTH));

    qDebug() << "[SessionDispatcher] Event received: Name=" << eventName << ", Id=" << eventId << ", Time=" << QTime::currentTime().toString();

    if (eventName == "REGISTER_VIEWER") {

        bool sessionExisting = false;
        // TODO: replace the temporary way to generate ID
        QString sessionID = QString::number(std::rand());
        NewServerConnector *connector = new NewServerConnector();

        CARTA::RegisterViewer registerViewer;
        registerViewer.ParseFromArray(message + EVENT_NAME_LENGTH + EVENT_ID_LENGTH, length - EVENT_NAME_LENGTH - EVENT_ID_LENGTH);
        if (registerViewer.session_id() != "") {
            sessionID = QString::fromStdString(registerViewer.session_id());
            qDebug() << "[SessionDispatcher] Get Session ID from frontend:" << sessionID;
            if (getConnectorInMap(sessionID)) {
                connector = static_cast<NewServerConnector*>(getConnectorInMap(sessionID));
                sessionExisting = true;
                qDebug() << "[SessionDispatcher] Session ID from frontend exists in backend:" << sessionID;
            }
        } else {
            qDebug() << "[SessionDispatcher] Use Session ID generated by backend:" << sessionID;
            setConnectorInMap(sessionID, connector);
        }

        sessionList[ws] = connector;

        if (!sessionExisting) {
            qRegisterMetaType<size_t>("size_t");
            qRegisterMetaType<uint32_t>("uint32_t");

            // start the image viewer
            connect(connector, SIGNAL(startViewerSignal(const QString &)),
                    connector, SLOT(startViewerSlot(const QString &)));

            // general commands
            connect(connector, SIGNAL(onBinaryMessageSignal(char*, size_t)),
                    connector, SLOT(onBinaryMessageSignalSlot(char*, size_t)));

            // open file
            connect(connector, SIGNAL(openFileSignal(uint32_t, QString, QString, int, int)),
                    connector, SLOT(openFileSignalSlot(uint32_t, QString, QString, int, int)));

            // set image view
            connect(connector, SIGNAL(setImageViewSignal(uint32_t, int , int, int, int, int, int, bool, int, int)),
                    connector, SLOT(setImageViewSignalSlot(uint32_t, int , int, int, int, int, int, bool, int, int)));

            // set image channel
            connect(connector, SIGNAL(imageChannelUpdateSignal(uint32_t, int, int, int)),
                    connector, SLOT(imageChannelUpdateSignalSlot(uint32_t, int, int, int)));

            // send binary signal to the frontend
            connect(connector, SIGNAL(jsBinaryMessageResultSignal(char*, QString, size_t)),
                    this, SLOT(forwardBinaryMessageResult(char*, QString, size_t)));

            //connect(connector, SIGNAL(onTextMessageSignal(QString)), connector, SLOT(onTextMessage(QString)));
            //connect(connector, SIGNAL(jsTextMessageResultSignal(QString)), this, SLOT(forwardTextMessageResult(QString)) );

            // create a simple thread
            QThread* newThread = new QThread();

            // let the new thread handle its events
            connector->moveToThread(newThread);
            newThread->setObjectName(sessionID);

            // start new thread's event loop
            newThread->start();

            //trigger signal
            emit connector->startViewerSignal(sessionID);
        }

        // add the message
        std::shared_ptr<CARTA::RegisterViewerAck> ack(new CARTA::RegisterViewerAck());
        ack->set_session_id(sessionID.toStdString());

        if (connector) {
            ack->set_success(true);
        } else {
            ack->set_success(false);
        }

        if (sessionExisting) {
            ack->set_session_type(::CARTA::SessionType::RESUMED);
        } else {
            ack->set_session_type(::CARTA::SessionType::NEW);
        }

        // serialize the message and send to the frontend
        QString respName = "REGISTER_VIEWER_ACK";
        PBMSharedPtr msg = ack;
        bool success = false;
        size_t requiredSize = 0;
        std::vector<char> result = serializeToArray(respName, eventId, msg, success, requiredSize);
        if (success) {
            ws->send(result.data(), requiredSize, uWS::OpCode::BINARY);
            qDebug() << "[SessionDispatcher] Send event:" << respName << QTime::currentTime().toString();
        }

        return;
    }

    NewServerConnector* connector= sessionList[ws];

    if (!connector) {
        qFatal("[SessionDispatcher] Cannot find the server connector");
        return;
    }

    if (connector != nullptr) {
        if (eventName == "OPEN_FILE") {

            CARTA::OpenFile openFile;
            openFile.ParseFromArray(message + EVENT_NAME_LENGTH + EVENT_ID_LENGTH, length - EVENT_NAME_LENGTH - EVENT_ID_LENGTH);
            QString fileDir = QString::fromStdString(openFile.directory());
            QString fileName = QString::fromStdString(openFile.file());
            int fileId = openFile.file_id();
            // If the histograms correspond to the entire current 2D image, the region ID has a value of -1.
            int regionId = -1;
            qDebug() << "[SessionDispatcher] Open the image file" << fileDir + "/" + fileName << "(fileId=" << fileId << ")";
            emit connector->openFileSignal(eventId, fileDir, fileName, fileId, regionId);

        } else if (eventName == "SET_IMAGE_VIEW") {

            CARTA::SetImageView viewSetting;
            viewSetting.ParseFromArray(message + EVENT_NAME_LENGTH + EVENT_ID_LENGTH, length - EVENT_NAME_LENGTH - EVENT_ID_LENGTH);
            int fileId = viewSetting.file_id();
            int mip = viewSetting.mip();
            int xMin = viewSetting.image_bounds().x_min();
            int xMax = viewSetting.image_bounds().x_max();
            int yMin = viewSetting.image_bounds().y_min();
            int yMax = viewSetting.image_bounds().y_max();
            qDebug() << "[SessionDispatcher] Set image bounds [x_min, x_max, y_min, y_max, mip]=["
                     << xMin << "," << xMax << "," << yMin << "," << yMax << "," << mip << "], fileId=" << fileId;

            int numSubsets = viewSetting.num_subsets();
            int precision = lround(viewSetting.compression_quality());
            bool isZFP = (viewSetting.compression_type() == CARTA::CompressionType::ZFP) ? true : false;

            emit connector->setImageViewSignal(eventId, fileId, xMin, xMax, yMin, yMax, mip, isZFP, precision, numSubsets);

        } else if (eventName == "SET_IMAGE_CHANNELS") {

            CARTA::SetImageChannels setImageChannels;
            setImageChannels.ParseFromArray(message + EVENT_NAME_LENGTH + EVENT_ID_LENGTH, length - EVENT_NAME_LENGTH - EVENT_ID_LENGTH);
            int fileId = setImageChannels.file_id();
            int channel = setImageChannels.channel();
            int stoke = setImageChannels.stokes();
            qDebug() << "[SessionDispatcher] Set image channel=" << channel << ", fileId=" << fileId << ", stoke=" << stoke;
            emit connector->imageChannelUpdateSignal(eventId, fileId, channel, stoke);

        } else {
            emit connector->onBinaryMessageSignal(message, length);
        }
    }
}

void SessionDispatcher::forwardTextMessageResult(QString result) {
    uWS::WebSocket<uWS::SERVER> *ws = nullptr;
    NewServerConnector* connector = qobject_cast<NewServerConnector*>(sender());
    std::map<uWS::WebSocket<uWS::SERVER>*, NewServerConnector*>::iterator iter;
    for (iter = sessionList.begin(); iter != sessionList.end(); ++iter) {
        if (iter->second == connector) {
            ws = iter->first;
            break;
        }
    }
    if (ws) {
        char *message = result.toUtf8().data();
        ws->send(message, result.toUtf8().length(), uWS::OpCode::TEXT);
    } else {
        qDebug() << "ERROR! Cannot find the corresponding websocket!";
    }
}

void SessionDispatcher::forwardBinaryMessageResult(char* message, QString respName, size_t length) {
    uWS::WebSocket<uWS::SERVER> *ws = nullptr;
    NewServerConnector* connector = qobject_cast<NewServerConnector*>(sender());
    std::map<uWS::WebSocket<uWS::SERVER>*, NewServerConnector*>::iterator iter;
    for (iter = sessionList.begin(); iter != sessionList.end(); ++iter){
        if (iter->second == connector){
            ws = iter->first;
            break;
        }
    }
    if (ws) {
        ws->send(message, length, uWS::OpCode::BINARY);
        qDebug() << "[SessionDispatcher] Send event:" << respName << QTime::currentTime().toString();
    } else {
        qDebug() << "[SessionDispatcher] ERROR! Cannot find the corresponding websocket!";
    }
}

IConnector* SessionDispatcher::getConnectorInMap(const QString & sessionID) {
    mutex.lock();
    auto iter = clientList.find(sessionID);

    if (iter != clientList.end()) {
        auto connector = iter->second;
        mutex.unlock();
        return connector;
    }

    mutex.unlock();
    return nullptr;
}

void SessionDispatcher::setConnectorInMap(const QString & sessionID, IConnector *connector) {
    mutex.lock();
    clientList[sessionID] = connector;
    mutex.unlock();
}

// //TODO implement later
// void SessionDispatcher::jsSendKeepAlive(){
// //    qDebug() << "get keepalive packet !!!!";
// }

//********* will comment the below later

void SessionDispatcher::initialize(const InitializeCallback & cb) {

}

void SessionDispatcher::setState(const QString& path, const QString & newValue) {

}

QString SessionDispatcher::getState(const QString & path) {
    return "";
}

QString SessionDispatcher::getStateLocation(const QString& saveName) const {
    return "";
}

IConnector::CallbackID SessionDispatcher::addCommandCallback (
        const QString & cmd,
        const IConnector::CommandCallback & cb) {
    return 0;
}

IConnector::CallbackID SessionDispatcher::addMessageCallback (
        const QString & cmd,
        const IConnector::MessageCallback & cb) {
    return 0;
}

IConnector::CallbackID SessionDispatcher::addStateCallback (
        IConnector::CSR path,
        const IConnector::StateChangedCallback & cb) {
    return 0;
}

void SessionDispatcher::registerView(IView * view) {

}

void SessionDispatcher::unregisterView(const QString& viewName) {

}

qint64 SessionDispatcher::refreshView(IView * view) {
    return 0;
}

void SessionDispatcher::removeStateCallback(const IConnector::CallbackID & /*id*/) {
    qFatal( "not implemented");
}

Carta::Lib::IRemoteVGView * SessionDispatcher::makeRemoteVGView(QString viewName) {
    return nullptr;
}
