#ifndef ICONNECTOR_H
#define ICONNECTOR_H

#include "IPlatform.h"

#include <memory>
#include <functional>
#include <cstdint>
#include <QString>

#include <google/protobuf/message_lite.h>

namespace Carta {
namespace Lib {
class IRemoteVGView;
}
}

/**
 * @brief The IConnector interface offers common API that the C++ code uses
 * to communicate with the JavaScript side. Different implementations are used
 * for desktop vs server-side versions.
 */

class IConnector {
public:

    /// alias for const QString &
    typedef const QString & CSR;

    /// shared pointer type of google protocol buffer messagelite
    typedef std::shared_ptr<google::protobuf::MessageLite> PBMSharedPtr;

    /// shared pointer type for convenience
    typedef std::shared_ptr< IConnector > SharedPtr;

    /// signature for command callback
    typedef std::function<QString(CSR cmd, CSR params, CSR sessionId)> CommandCallback;

    /// to distinguish with command callback, used to return results in protocol buffer formats
    typedef std::function<PBMSharedPtr (CSR cmd, CSR params, CSR sessionId)> MessageCallback;
    // Two possible definition of callback functions.
    // typedef std::function<std::vector<char> (CSR requestName, QString & responseName, const int requestID, PBMSharedPtr pb)> MessageCallback;
    // typedef std::function<PBMSharedPtr (CSR requestName, QString & responseName, const int requestID, PBMSharedPtr pb)> MessageCallback;

    /// signature for initialization callback
    typedef std::function<void(bool success)> InitializeCallback;

    /// signature for state changed callback
    typedef std::function<void (CSR path, CSR newValue)> StateChangedCallback;

    /// callback ID type (needed to remove callbacks)
    typedef int64_t CallbackID;

    /// add a callback for a command
    virtual CallbackID addCommandCallback( const QString & cmd, const CommandCallback & cb) = 0;

    /// similar to addCommandCallback, different callback function definition
    virtual CallbackID addMessageCallback( const QString & cmd, const MessageCallback & cb) = 0;

    /// add a callback for a tree change

    virtual IConnector* getConnectorInMap(const QString & sessionID) =0;

    virtual void setConnectorInMap(const QString & sessionID, IConnector *connector) = 0;

    virtual void startWebSocket() = 0;

    const size_t EVENT_NAME_LENGTH = 32;
    const size_t EVENT_ID_LENGTH = 4;

//    std::vector<char> serializeToArray(QString respName, uint32_t eventId, PBMSharedPtr msg, bool &success, size_t &requiredSize) {
//        success = false;
//        std::vector<char> result;
//        size_t messageLength = msg->ByteSize();
//        requiredSize = EVENT_NAME_LENGTH + EVENT_ID_LENGTH + messageLength;
//        if (result.size() < requiredSize) {
//            result.resize(requiredSize);
//        }
//        memset(result.data(), 0, EVENT_NAME_LENGTH);
//        memcpy(result.data(), respName.toStdString().c_str(), std::min<size_t>(respName.length(), EVENT_NAME_LENGTH));
//        memcpy(result.data() + EVENT_NAME_LENGTH, &eventId, EVENT_ID_LENGTH);
//        if (msg) {
//            msg->SerializeToArray(result.data() + EVENT_NAME_LENGTH + EVENT_ID_LENGTH, messageLength);
//            success = true;
//            //emit jsBinaryMessageResultSignal(result.data(), requiredSize);
//            //qDebug() << "Send event:" << respName << QTime::currentTime().toString();
//        }
//        return result;
//    }

    virtual ~IConnector() {}
};



#endif // ICONNECTOR_H
