#include "apiclient.h"

#include "qjsondocument.h"
#include "qnetworkproxy.h"

ApiClient::ApiClient(QString usernum, QObject* parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_usernum(usernum)
    , heartbeatTimer(new QTimer(this))
{
    initSocket();

    // 初始化心跳定时器
    connect(heartbeatTimer, &QTimer::timeout, this, [this]()
            {
        if (++missedHeartbeats >= MAX_MISSED_HEARTBEATS) {
            qDebug() << "Heart beat timeout, reconnecting...";
            reconnect();
        } else {
            // 发送心跳包
            QJsonObject heartbeat;
            heartbeat["tag"] = "heartbeat";
            sendJsonRequest(heartbeat);
        } });
}

ApiClient::~ApiClient()
{
    if (heartbeatTimer)
    {
        heartbeatTimer->stop();
        delete heartbeatTimer;
    }

    if (m_socket)
    {
        m_socket->disconnect();
        if (m_socket->state() == QAbstractSocket::ConnectedState)
        {
            m_socket->disconnectFromHost();
            if (m_socket->state() != QAbstractSocket::UnconnectedState)
            {
                m_socket->waitForDisconnected(1000);
            }
        }
        delete m_socket;
    }
}

void ApiClient::initSocket()
{
    if (m_socket)
    {
        m_socket->disconnect();
        delete m_socket;
    }

    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &ApiClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ApiClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &ApiClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &ApiClient::onError);
    connect(m_socket, &QTcpSocket::stateChanged, this, &ApiClient::onStateChanged);
}

bool ApiClient::connectToServer(const QString& host, quint16 port)
{
    m_host = host;
    m_port = port;

    if (QNetworkProxy::applicationProxy().type() != QNetworkProxy::NoProxy)
    {
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    }

    if (m_socket->state() == QAbstractSocket::ConnectedState)
    {
        return true;
    }

    m_socket->connectToHost(host, port);
    bool connected = m_socket->waitForConnected(CONNECT_TIMEOUT);

    if (connected)
    {
        heartbeatTimer->start(HEARTBEAT_INTERVAL);
        qDebug() << "Connected to server successfully";
    }
    else
    {
        qDebug() << "Connection failed:" << m_socket->errorString();
        emit connectionError(m_socket->errorString());
    }

    return connected;
}

bool ApiClient::isConnected()
{
    QMutexLocker locker(&socketMutex);
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void ApiClient::disconnect()
{
    QMutexLocker locker(&socketMutex);

    // 停止心跳定时器
    if (heartbeatTimer)
    {
        heartbeatTimer->stop();
    }

    // 清空消息队列和缓冲区
    messageQueue.clear();
    buffer.clear();
    isSending = false;

    if (m_socket)
    {
        // 断开所有信号连接
        m_socket->disconnect();

        // 如果套接字仍然处于连接状态，则断开连接
        if (m_socket->state() != QAbstractSocket::UnconnectedState)
        {
            m_socket->disconnectFromHost();

            // 等待断开连接完成
            if (m_socket->state() != QAbstractSocket::UnconnectedState)
            {
                m_socket->waitForDisconnected(CONNECT_TIMEOUT);
            }
        }

        // 重置心跳计数
        missedHeartbeats = 0;
    }
}

void ApiClient::reconnect()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        m_socket->disconnectFromHost();
    }

    clearBuffers();
    connectToServer(m_host, m_port);
}

void ApiClient::onError(QAbstractSocket::SocketError socketError)
{
    QString errorMsg = m_socket->errorString();
    qWarning() << "Socket error:" << errorMsg;
    emit connectionError(errorMsg);

    switch (socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        QTimer::singleShot(RECONNECT_TIMEOUT, this, &ApiClient::reconnect);
        break;
    case QAbstractSocket::SocketTimeoutError:
        reconnect();
        break;
    default:
        if (m_socket->state() != QAbstractSocket::ConnectedState)
        {
            QTimer::singleShot(RECONNECT_TIMEOUT, this, &ApiClient::reconnect);
        }
        break;
    }
}

void ApiClient::onStateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << "Socket state changed to:" << socketState;
}

void ApiClient::clearBuffers()
{
    QMutexLocker locker(&socketMutex);
    buffer.clear();
    messageQueue.clear();
    isSending = false;
}

void ApiClient::onConnected()
{
    missedHeartbeats = 0;
    emit connected();
}

void ApiClient::onDisconnected()
{
    heartbeatTimer->stop();
    clearBuffers();
    emit disconnected();
}

void ApiClient::sendJsonRequest(const QJsonObject& responseJson)
{
    QMutexLocker locker(&socketMutex);

    // 将消息转换为 JSON 格式并添加到消息队列
    QByteArray jsonData = QJsonDocument(responseJson).toJson() + "END";
    messageQueue.enqueue(jsonData);

    // 处理消息队列
    processMessageQueue();
}

void ApiClient::processMessageQueue()
{
    // 确保队列中有消息且当前未发送中
    if (messageQueue.isEmpty() || isSending)
    {
        return;
    }

    // 标记当前正在发送
    isSending = true;

    // 从队列中取出消息
    QByteArray message = messageQueue.dequeue();

    // 写入套接字并等待数据写入完成
    m_socket->write(message);
    if (!m_socket->waitForBytesWritten())
    {
        qCritical() << "Failed to write message to socket.";
        isSending = false;
        return;
    }
    if (message.size() > 500)
        qDebug() << "Message sent successfully:" << message.size();
    else
        qDebug() << "Message sent successfully:" << message;

    // 标记发送完成，继续处理队列中的下一条消息
    isSending = false;
    processMessageQueue();
}

QString ApiClient::getUsernum() const
{
    return m_usernum;
}

void ApiClient::onReadyRead()
{
    // 从套接字读取所有可用数据并追加到缓存区
    buffer += m_socket->readAll();

    // 检查缓存区中是否包含完整的消息（以 "END" 为分隔符）
    while (true)
    {
        int endIndex = buffer.indexOf("\n}\nEND");
        if (endIndex == -1)
        {
            break; // 未找到完整消息，等待更多数据
        }

        // 提取完整消息
        QByteArray completeMessage = buffer.left(endIndex + 3);
        buffer.remove(0, endIndex + 6); // 移除已处理的消息

        if (completeMessage.size() > 500)
            qDebug() << "Received data (size):" << completeMessage.size();
        else
            qDebug() << "Received data (content):" << completeMessage;
        // 尝试解析 JSON 数据
        QJsonDocument jsonDoc = QJsonDocument::fromJson(completeMessage);
        if (!jsonDoc.isNull() && jsonDoc.isObject())
        {
            QJsonObject jsonObj = jsonDoc.object();
            if (jsonObj["tag"] == "heartbeat")
            {
                // 心跳更新
                missedHeartbeats = 0;
            }
            else if (jsonObj["tag"] == "shutdown")
            {
                if (m_usernum != nullptr)
                {
                    jsonObj["tag"] = "home";
                    jsonObj["mode"] = "shutdown";
                    emit dataReceived(jsonObj);
                }
            }
            else
            {
                // 其它模块
                emit dataReceived(jsonObj); // 发送信号处理数据
            }
        }
        else
        {
            qCritical() << "Invalid JSON message received:" << completeMessage;
        }
    }
}
