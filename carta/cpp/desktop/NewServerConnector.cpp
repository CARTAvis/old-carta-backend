/**
 *
 **/

#include "NewServerConnector.h"

#include <iostream>
#include <QXmlInputSource>
#include <cmath>
#include <QTime>
#include <QTimer>
#include <QCoreApplication>
#include <functional>
#include <QStringList>
#include <QBuffer>
#include <QThread>

NewServerConnector::NewServerConnector():
    m_dataLoader(nullptr) {
    m_callbackNextId = 0;
    _makeDataLoader();
}

NewServerConnector::~NewServerConnector()
{
}

IConnector::CallbackID NewServerConnector::addCommandCallback(
        const QString & cmd,
        const IConnector::CommandCallback & cb)
{
    m_commandCallbackMap[cmd].push_back( cb);
    return m_callbackNextId++;
}

IConnector::CallbackID NewServerConnector::addMessageCallback(
        const QString & cmd,
        const IConnector::MessageCallback & cb)
{
    m_messageCallbackMap[cmd].push_back( cb);
    return m_callbackNextId++;
}

IConnector* NewServerConnector::getConnectorInMap(const QString & sessionID){
    return nullptr;
}

void NewServerConnector::setConnectorInMap(const QString & sessionID, IConnector *connector){
}

void NewServerConnector::startWebSocket(){
    qCritical("NewServerConnector should not start a websocket!");
    CARTA_ASSERT_X( false, "NewServerConnector should not start a websocket!");
}

void NewServerConnector::startViewerSlot(const QString & sessionID) {

    QString name = QThread::currentThread()->objectName();
    qDebug() << "[NewServerConnector] Current thread name:" << name;
    if (name != sessionID) {
        qDebug()<< "ignore startViewerSlot";
        return;
    }

    viewer.start();
}

void NewServerConnector::onTextMessage(QString message){
    QString controllerID = this->viewer.m_viewManager->registerView("pluginId:ImageViewer,index:0");
    QString cmd = controllerID + ":" + message;

    qDebug() << "Message received:" << message;
    auto & allCallbacks = m_messageCallbackMap[ message];

    // QString result;
    std::string data;
    PBMSharedPtr msg;

    for( auto & cb : allCallbacks ) {
        msg = cb( message, "", "1");
    }
    msg->SerializeToString(&data);
    const QString result = QString::fromStdString(data);

    if( allCallbacks.size() == 0) {
        qWarning() << "JS command has no server listener:" << message;
    }

    emit jsTextMessageResultSignal(result);
}

void NewServerConnector::onBinaryMessageSignalSlot(const char *message, size_t length){
    if (length < EVENT_NAME_LENGTH + EVENT_ID_LENGTH){
        qWarning("Illegal message.");
        return;
    }

    size_t nullIndex = 0;
    for (size_t i = 0; i < EVENT_NAME_LENGTH; i++) {
        if (!message[i]) {
            nullIndex = i;
            break;
        }
    }
    QString eventName = QString::fromStdString(std::string(message, nullIndex));

    uint32_t eventId = *((uint32_t*) (message + EVENT_NAME_LENGTH));

    qDebug() << "[NewServerConnector] Event received: Name=" << eventName << ", Id=" << eventId << ", Time=" << QTime::currentTime().toString();

    QString respName;
    PBMSharedPtr msg;

    if (eventName == "REGISTER_VIEWER") {
        // The message should be handled in sessionDispatcher
        qWarning("Illegal request in NewServerConnector. Please handle it in SessionDispatcher.");
        return;

    } else if (eventName == "CLOSE_FILE") {

        CARTA::CloseFile closeFile;
        closeFile.ParseFromArray(message + EVENT_NAME_LENGTH + EVENT_ID_LENGTH, length - EVENT_NAME_LENGTH - EVENT_ID_LENGTH);
        int closeFileId = closeFile.file_id();
        qDebug() << "[NewServerConnector] Close the file id=" << closeFileId;

    } else {
        // Insert non-global object id
        // QString controllerID = this->viewer.m_viewManager->registerView("pluginId:ImageViewer,index:0");
        // QString cmd = controllerID + ":" + eventName;
        qCritical() << "[NewServerConnector] There is no event handler:" << eventName;

//        auto & allCallbacks = m_messageCallbackMap[eventName];
//
//        if (allCallbacks.size() == 0) {
//            qCritical() << "There is no event handler:" << eventName;
//            return;
//        }
//
//        for (auto & cb : allCallbacks) {
//            msg = cb(eventName, "", "1");
//        }

        return;
    }

    // socket->send(binaryPayloadCache.data(), requiredSize, uWS::BINARY);
    // emit jsTextMessageResultSignal(result);
    return;
}

void NewServerConnector::fileListRequestSignalSlot(uint32_t eventId, CARTA::FileListRequest fileListRequest) {
    PBMSharedPtr msg = m_dataLoader->getFileList(fileListRequest);

    // send the serialized message to the frontend
    sendSerializedMessage("FILE_LIST_RESPONSE", eventId, msg);
}

void NewServerConnector::fileInfoRequestSignalSlot(uint32_t eventId, CARTA::FileInfoRequest fileInfoRequest) {
    PBMSharedPtr msg = m_dataLoader->getFileInfo(fileInfoRequest);

    // send the serialized message to the frontend
    sendSerializedMessage("FILE_INFO_RESPONSE", eventId, msg);
}

void NewServerConnector::openFileSignalSlot(uint32_t eventId, QString fileDir, QString fileName, int fileId, int regionId) {
    QString respName;
    PBMSharedPtr msg;

    respName = "OPEN_FILE_ACK";

    Carta::Data::Controller* controller = _getController();

    if (!QDir(fileDir).exists()) {
        qWarning() << "[NewServerConnector] File directory doesn't exist! (" << fileDir << ")";
        return;
    }

    qDebug() << "[NewServerConnector] Open the file ID:" << fileId;

    bool success;
    controller->addData(fileDir + "/" + fileName, &success, fileId);

    std::shared_ptr<Carta::Lib::Image::ImageInterface> image = controller->getImage();

    CARTA::FileInfo* fileInfo = new CARTA::FileInfo();
    fileInfo->set_name(fileName.toStdString());

    if (image->getType() == "FITSImage") {
        fileInfo->set_type(CARTA::FileType::FITS);
    } else {
        fileInfo->set_type(CARTA::FileType::CASA);
    }
    // fileInfo->add_hdu_list(openFile.hdu());

    const std::vector<int> dims = image->dims();
    CARTA::FileInfoExtended* fileInfoExt = new CARTA::FileInfoExtended();
    fileInfoExt->set_dimensions(dims.size());
    fileInfoExt->set_width(dims[0]);
    fileInfoExt->set_height(dims[1]);

    int stokeIndicator = controller->getStokeIndicator();
    // set the stoke axis if it exists
    if (stokeIndicator > 0) { // if stoke axis exists
        if (dims[stokeIndicator] > 0) { // if stoke dimension > 0
            fileInfoExt->set_stokes(dims[stokeIndicator]);
        }
    }

    // for the dims[k] that is not the stoke frame nor the x- or y-axis,
    // we assume it is a depth (it is the Spectral axis or the other unmarked axis)
    int lastFrame = 0;
    if (dims.size() > 2) {
        for (int i = 2; i < dims.size(); i++) {
            if (i != stokeIndicator && dims[i] > 0) {
                fileInfoExt->set_depth(dims[i]);
                lastFrame = dims[i] - 1;
                break;
            }
        }
    }
    m_lastFrame[fileId] = lastFrame;

    // FileInfoExtended: return all entries (MUST all) for frontend to render (AST)
    if (false == m_dataLoader->getFitsHeaders(fileInfoExt, image)) {
        qDebug() << "[NewServerConnector] Get fits headers error!";
    }

    // we cannot handle the request so far, return a fake response.
    std::shared_ptr<CARTA::OpenFileAck> ack(new CARTA::OpenFileAck());
    ack->set_success(true);
    ack->set_file_id(fileId);
    ack->set_allocated_file_info(fileInfo);
    ack->set_allocated_file_info_extended(fileInfoExt);
    msg = ack;

    // send the serialized message to the frontend
    sendSerializedMessage(respName, eventId, msg);

    // set the initial image bounds
    m_imageBounds[fileId] = {0, 0, 0, 0, 0}; // {x_min, x_max, y_min, y_max, mip}

    // set the initial zfp
    m_isZFP[fileId] = false;
    m_ZFPSet[fileId] = {0, 0};

    // set the initial channel for spectral and stoke frames
    m_currentChannel[fileId] = {0, 0}; // {frameLow, stokeFrame}

    // set spectral and stoke frame ranges to calculate the pixel to histogram data
    //m_calHistRange[fileId] = {0, m_lastFrame[fileId], 0}; // {frameLow, frameHigh, stokeFrame}
    //m_calHistRange[fileId] = {0, 0, 0}; // {frameLow, frameHigh, stokeFrame}

    // set image changed is true
    m_changeFrame[fileId] = true;
}

void NewServerConnector::setImageViewSignalSlot(uint32_t eventId, int fileId, int xMin, int xMax, int yMin, int yMax, int mip,
    bool isZFP, int precision, int numSubsets) {
    QString respName = "RASTER_IMAGE_DATA";

    // check if the boundaries are valid
    if (xMin > xMax || yMin > yMax) {
        qWarning() << "[NewServerConnector] Invalid image bound [xMin, xMax, yMin, yMax]: [" << xMin << ", " << xMax << ", " << yMin << ", " << yMax << "]";
        return;
    }

    // check if need to reset image bounds
    if (xMin != m_imageBounds[fileId][0] || xMax != m_imageBounds[fileId][1] ||
        yMin != m_imageBounds[fileId][2] || yMax != m_imageBounds[fileId][3] ||
        mip != m_imageBounds[fileId][4]) {
        // update image viewer bounds with respect to the fileId
        m_imageBounds[fileId] = {xMin, xMax, yMin, yMax, mip};
    } else { // Frontend image viewer signal is repeated, just ignore the signal
        return;
    }

    // check if need to reset ZFP parameters
    if (isZFP != m_isZFP[fileId]) {
        m_isZFP[fileId] = isZFP;
        m_ZFPSet[fileId] = {precision, numSubsets};
    }

    // get the controller
    Carta::Data::Controller* controller = _getController();

    // set the file id as the private parameter in the Stack object
    controller->setFileId(fileId);

    // set the current channel
    int frameLow = m_currentChannel[fileId][0];
    int frameHigh = frameLow;
    int stokeFrame = m_currentChannel[fileId][1];

    // If the histograms correspond to the entire current 2D image, the region ID has a value of -1.
    int regionId = -1;

    // do not include unit converter for pixel values
    Carta::Lib::IntensityUnitConverter::SharedPtr converter = nullptr;

    // get the down sampling raster image raw data
    PBMSharedPtr raster = controller->getRasterImageData(fileId, xMin, xMax, yMin, yMax, mip,
                                                         frameLow, frameHigh, stokeFrame,
                                                         isZFP, precision, numSubsets,
                                                         m_changeFrame[fileId], regionId, numberOfBins, converter);

    // send the serialized message to the frontend
    sendSerializedMessage(respName, eventId, raster);
}

void NewServerConnector::imageChannelUpdateSignalSlot(uint32_t eventId, int fileId, int channel, int stoke) {
    QString respName = "RASTER_IMAGE_DATA";

    if (m_currentChannel[fileId][0] != channel || m_currentChannel[fileId][1] != stoke) {
        //qDebug() << "[NewServerConnector] Set image channel=" << channel << ", fileId=" << fileId << ", stoke=" << stoke;
        // update the current channel and stoke
        m_currentChannel[fileId] = {channel, stoke};
    } else {
        //qDebug() << "[NewServerConnector] Internal signal is repeated!! Don't know the reason yet, just ignore the signal!!";
        return;
    }

    // set spectral and stoke frame ranges to calculate the pixel to histogram data
    //m_calHistRange[fileId] = {0, m_lastFrame[fileId], 0};
    //m_calHistRange[fileId] = {channel, channel, stoke};

    // get the controller
    Carta::Data::Controller* controller = _getController();

    // set the file id as the private parameter in the Stack object
    controller->setFileId(fileId);

    // set the current channel
    int frameLow = m_currentChannel[fileId][0];
    int frameHigh = frameLow;
    int stokeFrame = m_currentChannel[fileId][1];

    // If the histograms correspond to the entire current 2D image, the region ID has a value of -1.
    int regionId = -1;

    // do not include unit converter for pixel values
    Carta::Lib::IntensityUnitConverter::SharedPtr converter = nullptr;

    // set image changed is true
    m_changeFrame[fileId] = true;

    // get image viewer bounds with respect to the fileId
    int xMin = m_imageBounds[fileId][0];
    int xMax = m_imageBounds[fileId][1];
    int yMin = m_imageBounds[fileId][2];
    int yMax = m_imageBounds[fileId][3];
    int mip = m_imageBounds[fileId][4];

    bool isZFP = m_isZFP[fileId];
    int precision = m_ZFPSet[fileId][0];
    int numSubsets = m_ZFPSet[fileId][1];

    // use image bounds with respect to the fileID and get the down sampling raster image raw data
    PBMSharedPtr raster = controller->getRasterImageData(fileId, xMin, xMax, yMin, yMax, mip,
                                                         frameLow, frameHigh, stokeFrame,
                                                         isZFP, precision, numSubsets,
                                                         m_changeFrame[fileId], regionId, numberOfBins, converter);

    // send the serialized message to the frontend
    sendSerializedMessage(respName, eventId, raster);
}

void NewServerConnector::setCursorSignalSlot(uint32_t eventId, int fileId, CARTA::Point point, CARTA::SetSpatialRequirements setSpatialReqs) {
    qDebug() << "[NewServerConnector] set cursor file id=" << fileId;

    // Part 1: Caculate spatial profile data
    // get the controller
    Carta::Data::Controller* controller = _getController();

    // set the file id as the private parameter in the Stack object
    controller->setFileId(fileId);

    // set the current channel
    int frameLow = m_currentChannel[fileId][0];
    int frameHigh = frameLow;
    int stokeFrame = m_currentChannel[fileId][1];

    // do not include unit converter for pixel values
    Carta::Lib::IntensityUnitConverter::SharedPtr converter = nullptr;

    // get X/Y profile & fill in the response
    int x = (int)round(point.x());
    int y = (int)round(point.y());
    PBMSharedPtr pbMsg = controller->getXYProfiles(fileId, x, y, frameLow, frameHigh, stokeFrame, converter);

    // send the serialized message to the frontend
    sendSerializedMessage("SPATIAL_PROFILE_DATA", eventId, pbMsg);

    // skip spectral profile if there is only single channel in the image
    // channel numbers = dims[spectralIndicator]
    int spectralIndicator = controller->getSpectralIndicator();
    std::vector<int> dims = controller->getImageDimensions();

    if(0 <= spectralIndicator && 1 < dims[spectralIndicator]) {
        // get spectral profile
        pbMsg = controller->getSpectralProfile(fileId, x, y, stokeFrame);

        // send the serialized message to the frontend
        sendSerializedMessage("SPECTRAL_PROFILE_DATA", eventId, pbMsg);
    }
}

void NewServerConnector::setSpatialRequirementsSignalSlot(uint32_t eventId, int fileId, int regionId, google::protobuf::RepeatedPtrField<std::string> spatialProfiles) {
    // get the controller
    Carta::Data::Controller* controller = _getController();

    // set the file id as the private parameter in the Stack object
    controller->setFileId(fileId);

    if (controller->setSpatialRequirements(fileId, regionId, spatialProfiles)) {
        qDebug() << "[NewServerConnector] set spatial requirement successfully.";
    } else {
        qDebug() << "[NewServerConnector] set spatial requirement failed!";
    }
}

void NewServerConnector::setSpectralRequirementsSignalSlot(uint32_t eventId, int fileId, int regionId, google::protobuf::RepeatedPtrField<CARTA::SetSpectralRequirements_SpectralConfig> spectralProfiles) {
    // get the controller
    Carta::Data::Controller* controller = _getController();

    // set the file id as the private parameter in the Stack object
    controller->setFileId(fileId);

    // set the current channel
    int stoke = m_currentChannel[fileId][1];

    if (controller->setSpectralRequirements(fileId, regionId, stoke, spectralProfiles)) {
        qDebug() << "[NewServerConnector] set spectral requirement successfully.";
    } else {
        qDebug() << "[NewServerConnector] set spectral requirement failed!";
    }
}

void NewServerConnector::sendSerializedMessage(QString respName, uint32_t eventId, PBMSharedPtr msg) {
    if (nullptr == msg) {
        qWarning() << "Respond message is nullptr. Respond: " << respName << ", eventId: " << eventId;
        return;
    }

    emit jsBinaryMessageResultSignal(respName, eventId, msg);
}

Carta::Data::Controller* NewServerConnector::_getController() {
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    QString controllerID = this->viewer.m_viewManager->registerView("pluginId:ImageViewer,index:0").split("/").last();
    qDebug() << "[NewServerConnector] controllerID=" << controllerID;
    Carta::Data::Controller* controller = dynamic_cast<Carta::Data::Controller*>( objMan->getObject(controllerID) );

    return controller;
}

void NewServerConnector::_makeDataLoader() {
    if ( m_dataLoader == nullptr ){
        Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
        m_dataLoader = objMan->createObject<Carta::Data::DataLoader>();
    }
}

