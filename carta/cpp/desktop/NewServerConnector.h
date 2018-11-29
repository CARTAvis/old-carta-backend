/**
 *
 **/

#ifndef NEW_SERVER_CONNECTOR_H
#define NEW_SERVER_CONNECTOR_H

#include <QObject>
#include <QList>
#include <QByteArray>

#include "CartaLib/IPercentileCalculator.h"

#include "core/IConnector.h"
#include "core/CallbackList.h"
#include "core/Viewer.h"
#include "core/MyQApp.h"
#include "core/State/ObjectManager.h"
#include "core/Data/DataLoader.h"
#include "core/Data/ViewManager.h"
#include "core/Data/Image/Controller.h"
#include "core/Data/Image/DataSource.h"

#include "CartaLib/Proto/open_file.pb.h"
#include "CartaLib/Proto/set_image_view.pb.h"
#include "CartaLib/Proto/spectral_profile.pb.h"
#include "CartaLib/Proto/spatial_profile.pb.h"
#include "CartaLib/Proto/set_cursor.pb.h"
#include "CartaLib/Proto/region_stats.pb.h"
#include "CartaLib/Proto/region_requirements.pb.h"
#include "CartaLib/Proto/region.pb.h"
#include "CartaLib/Proto/error.pb.h"
#include "CartaLib/Proto/contour_image.pb.h"
#include "CartaLib/Proto/contour.pb.h"
#include "CartaLib/Proto/close_file.pb.h"
#include "CartaLib/Proto/animation.pb.h"

class NewServerConnector : public QObject, public IConnector
{
    Q_OBJECT
public:

    /// constructor
    explicit NewServerConnector();

    // implementation of IConnector interface
    virtual void initialize( const InitializeCallback & cb) override;
    virtual void setState(const QString& state, const QString & newValue) override;
    virtual CallbackID addCommandCallback( const QString & cmd, const CommandCallback & cb) override;
    virtual CallbackID addMessageCallback( const QString & cmd, const MessageCallback & cb) override;
    virtual void removeStateCallback( const CallbackID & id) override;
    virtual Carta::Lib::IRemoteVGView * makeRemoteVGView( QString viewName) override;

    /// Return the location where the state is saved.
    virtual QString getStateLocation( const QString& saveName ) const override;

     ~NewServerConnector();

    Viewer viewer;
    QThread *selfThread; //not really use now, may take effect later

public slots:

    void startViewerSlot(const QString & sessionID);
    void onTextMessage(QString message);
    void onBinaryMessageSignalSlot(const char* message, size_t length);
    void sendSerializedMessage(QString respName, uint32_t eventId, PBMSharedPtr msg);

    void imageChannelUpdateSignalSlot(uint32_t eventId, int fileId, int channel, int stoke);
    void setImageViewSignalSlot(uint32_t eventId, int fileId, int xMin, int xMax, int yMin, int yMax, int mip,
                                bool isZFP, int precision, int numSubsets);
    void openFileSignalSlot(uint32_t eventId, QString fileDir, QString fileName, int fileId, int regionId);
    void setCursorSignalSlot(uint32_t eventId, int fileId, CARTA::Point point, CARTA::SetSpatialRequirements setSpatialReqs);
    void setSpatialRequirementsSignalSlot(uint32_t eventId, int fileId, int regionId, google::protobuf::RepeatedPtrField<std::string> spatialProfiles);
    void setSpectralRequirementsSignalSlot(uint32_t eventId, int fileId, int regionId, google::protobuf::RepeatedPtrField<CARTA::SetSpectralRequirements_SpectralConfig> spectralProfiles);

    void fileListRequestSignalSlot(uint32_t eventId, CARTA::FileListRequest fileListRequest);
    void fileInfoRequestSignalSlot(uint32_t eventId, CARTA::FileInfoRequest fileInfoRequest);

signals:

    //grimmer: newArch will not use stateChange mechanism anymore

    //new arch
    void startViewerSignal(const QString & sessionID);
    void onTextMessageSignal(QString message);
    void onBinaryMessageSignal(const char* message, size_t length);

    void jsTextMessageResultSignal(QString result);
    void jsBinaryMessageResultSignal(QString respName, uint32_t eventId, PBMSharedPtr message);

    void imageChannelUpdateSignal(uint32_t eventId, int fileId, int channel, int stoke);
    void setImageViewSignal(uint32_t eventId, int fileId, int xMin, int xMax, int yMin, int yMax, int mip,
                            bool isZFP, int precision, int numSubsets);
    void openFileSignal(uint32_t eventId, QString fileDir, QString fileName, int fileId, int regionId);
    void setCursorSignal(uint32_t eventId, int fileId, CARTA::Point point, CARTA::SetSpatialRequirements setSpatialReqs);
    void setSpatialRequirementsSignal(uint32_t eventId, int fileId, int regionId, google::protobuf::RepeatedPtrField<std::string> spatialProfiles);
    void setSpectralRequirementsSignal(uint32_t eventId, int fileId, int regionId, google::protobuf::RepeatedPtrField<CARTA::SetSpectralRequirements_SpectralConfig> spectralProfiles);

    void fileListRequestSignal(uint32_t eventId, CARTA::FileListRequest fileListRequest);
    void fileInfoRequestSignal(uint32_t eventId, CARTA::FileInfoRequest fileInfoRequest);

public:

    typedef std::vector<CommandCallback> CommandCallbackList;
    std::map<QString,  CommandCallbackList> m_commandCallbackMap;

    typedef std::vector<MessageCallback> MessageCallbackList;
    std::map<QString,  MessageCallbackList> m_messageCallbackMap;

    // list of callbacks
    typedef CallbackList<CSR, CSR> StateCBList;

    /// IDs for command callbacks
    CallbackID m_callbackNextId;

    IConnector* getConnectorInMap(const QString & sessionID) override;
    void setConnectorInMap(const QString & sessionID, IConnector *connector) override;

    void startWebSocket() override;

    /// @todo move as may of these as possible to protected section

protected:

    InitializeCallback m_initializeCallback;

    Carta::Data::Controller* _getController();

private:

    std::map<int, std::vector<int> > m_imageBounds; // m_imageBounds[fileId] = {x_min, x_max, y_min, y_max, mip}
    std::map<int, bool> m_isZFP; // whether if ZFP compression is required by the frontend
    std::map<int, std::vector<int> > m_ZFPSet; // m_ZFPSet[fileId] = {precision, numSubsets}
    std::map<int, std::vector<int> > m_currentChannel; // m_currentChannel[fileId] = {spectralFrame, stokeFrame}
    //std::map<int, std::vector<int> > m_calHistRange; // m_calHistRange[fileId] = {frameLow, frameHigh, stokeFrame}
    std::map<int, int> m_lastFrame; // m_lastFrame[fileId] = lastFrame (for the spectral axis)
    std::map<int, bool> m_changeFrame;
    const int numberOfBins = 10000; // define number of bins for calculating pixels to histogram data
};


#endif // NEW_SERVER_CONNECTOR_H
