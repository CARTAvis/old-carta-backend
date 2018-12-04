/**
 *
 **/


#ifndef SESSION_DISPATCHER_H
#define SESSION_DISPATCHER_H

#include <QObject>
#include <qmutex.h>
#include "NewServerConnector.h"

#include "core/IConnector.h"
#include "QtWebSockets/qwebsocketserver.h"
#include "QtWebSockets/qwebsocket.h"

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)

QT_FORWARD_DECLARE_CLASS(WebSocketClientWrapper)
QT_FORWARD_DECLARE_CLASS(QWebChannel)

class SessionDispatcher : public QObject, public IConnector
{
    Q_OBJECT
public:

    /// constructor
    explicit SessionDispatcher();

    //** will comment later
    // implementation of IConnector interface
    virtual CallbackID addCommandCallback( const QString & cmd, const CommandCallback & cb) override;
    virtual CallbackID addMessageCallback( const QString & cmd, const MessageCallback & cb) override;
    //**

    void startWebSocket() override;
    ~SessionDispatcher();

public:

    IConnector* getConnectorInMap(const QString & sessionID) override;
    void setConnectorInMap(const QString & sessionID, IConnector *connector) override;

protected:

    std::map<QString, IConnector*> clientList;

private:

    QMutex mutex;
    QWebSocketServer *m_pWebSocketServer;
    // prevent being accessed by other to avoid thread-safety problem
    std::map<QWebSocket*, NewServerConnector*> sessionList;

    std::vector<char> _serializeToArray(QString respName, uint32_t eventId, PBMSharedPtr msg, bool &success, size_t &requiredSize);

private slots:

    void onNewConnection();
    void onTextMessage(QString);
    void onBinaryMessage(QByteArray qByteMessage);
    void forwardTextMessageResult(QString);
    void forwardBinaryMessageResult(QString respName, uint32_t eventId, PBMSharedPtr protoMsg);
};


#endif // SESSION_DISPATCHER_H
