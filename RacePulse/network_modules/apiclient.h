#ifndef APICLIENT_H
#define APICLIENT_H

#include <qqueue.h>

#include <QMutex>
#include <QObject>
#include <QTcpSocket>

#include "qjsonobject.h"
#include "qtimer.h"

class ApiClient : public QObject
{
    Q_OBJECT

public:
    explicit ApiClient(QString usernum, QObject* parent = nullptr);
    ~ApiClient();

    bool connectToServer(const QString& host = "127.0.0.1", quint16 port = 10086);
    void sendJsonRequest(const QJsonObject& responseJson);
    QString getUsernum() const; // 添加 const 修饰符

    // 添加新的公共方法
    bool isConnected();
    void disconnect();
    void reconnect();

signals:
    void connected();
    void disconnected();
    void dataReceived(const QJsonObject& data);
    void connectionError(const QString& errorMessage);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);
    void onStateChanged(QAbstractSocket::SocketState socketState);

private:
    void processMessageQueue();
    void initSocket();
    void clearBuffers();

    // 成员变量
    QTcpSocket* m_socket;
    QString m_usernum;
    QByteArray buffer;
    QQueue<QByteArray> messageQueue;
    QMutex socketMutex;
    bool isSending{false};

    // 连接相关配置
    QString m_host;
    quint16 m_port;
    static constexpr int RECONNECT_TIMEOUT = 5000;           // 重连超时时间(ms)
    static constexpr int CONNECT_TIMEOUT = 1000;             // 连接超时时间(ms)
    static constexpr int MAX_BUFFER_SIZE = 10 * 1024 * 1024; // 最大缓冲区大小(10MB)

    // 心跳检测
    QTimer* heartbeatTimer;
    int missedHeartbeats{0};
    static constexpr int MAX_MISSED_HEARTBEATS = 3;
    static constexpr int HEARTBEAT_INTERVAL = 5000;
};

#endif // APICLIENT_H
