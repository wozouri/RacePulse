#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QDir>
#include <QJsonDocument>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QRandomGenerator>
#include <QReadWriteLock>
#include <QTcpSocket>
#include <QTimer>
#include <opencv2/opencv.hpp>

#include "connectionpool.h"

class Server;

class ClientHandler : public QObject
{
    Q_OBJECT

public:
    // Constructor & Destructor
    ClientHandler(QTcpSocket* socket, ConnectionPool& pool, Server* srv);
    ~ClientHandler();

    // Database operations
    bool databasesConnect();

    // Socket handling
    void onReadyRead();
    void onDisconnected();
    void closeConnection();
    void handleSocketError(QAbstractSocket::SocketError error);

    // JSON message processing
    void processRequest(const QJsonObject& jsonObj);
    void receiveMessage(const QJsonObject& json);
    void notifyClientShutdown(const QJsonObject& json); // 服务器关闭通知客户端
    void sendJsonResponse(const QJsonObject& responseJson);
    void processMessageQueue();
    void sendErrorResponse(QJsonObject qjsonObj, const QString& reason);
    void cleanup();

    // Client request handlers
    void dealLogin(const QJsonObject& json);
    void dealRegister(const QJsonObject& json);
    void dealSearchTerm(const QJsonObject& json);
    void dealContainsFace(const QJsonObject& json);
    void dealCheckFace(const QJsonObject& json);
    void dealUpdateFace(const QJsonObject& json);

    // Client-to-client communication
    void forwordKickedOffline(const QJsonObject& json);

    // Face recognition utilities
    cv::Mat QImageToCvMat(const QImage& Image);
    cv::Mat extractFeatureVector(const cv::Mat& face);
    bool verifyIdentity(const cv::Mat& inputFeature, const std::string& filename);
    bool saveFeatureVector(const cv::Mat& featureVector, const std::string& filename);

    bool insertUserRecord(const QString& usernum, const QString& password, const QString& nickname, const QString& avatar);
    QString handleAvatar(const QString& usernum, const QJsonObject& json);
    QString generateUniqueUsernum();
    bool checkNicknameAvailable(const QString& nickname);

    QString loadAvatarAsBase64(const QString &avatarFilename);
signals:
    void dataReceived(const QJsonObject& jsonObject);

    void sendMessage(const QJsonObject& jsonObject);

private:
    // Synchronization
    QMutex dbMutex;
    QMutex socketMutex;
    QReadWriteLock lock;

    // Network
    QTcpSocket* m_socket;
    QQueue<QByteArray> messageQueue;
    bool isSending{false};
    QByteArray buffer;

    // Database
    QSqlDatabase db;
    Server* srv;
    ConnectionPool& pool;

    // JSON
    QJsonDocument jsonDoc;

    // User data
    QString randomNumber;
    QString account{"0"};

    // Face recognition
    cv::CascadeClassifier faceCascade;

    // Heartbeat
    QTimer* heartbeatTimer;
    int missedHeartbeats;
    static constexpr int MAX_MISSED_HEARTBEATS = 3;
    static constexpr int HEARTBEAT_INTERVAL = 5000;
};

#endif // CLIENTHANDLER_H
