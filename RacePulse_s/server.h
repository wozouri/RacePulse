#ifndef SERVER_H
#define SERVER_H

#include <ConnectionPool.h>
#include <QSqlDatabase.h>

#include <QTcpServer>
#include <QTcpSocket>
#include <QThreadPool>
#include <QWidget>

#include "clienthandler.h"
#include "qmutex.h"

const qint16 port = 10086;

class ClientHandler;
namespace Ui
{
class Server;
}

class Server : public QWidget
{
    Q_OBJECT

public:
    explicit Server(QWidget* parent = nullptr);
    ~Server();
    bool tcpListen();

    void onNewConnection();

    bool databaseConnect();

    void showtable(const QString& tablename);

private slots:
    void on_pu_listen_clicked();

    void on_pu_refresh_table_clicked();

    void on_combo_table_currentIndexChanged(int index);

public:
    QHash<QString, std::shared_ptr<ClientHandler>> clientsMap; // 存储账号与ClientHandler的映射 共享资源
    QList<std::shared_ptr<ClientHandler>> activeHandlers;

    void notifyClientsAndClose();

    void addClient(const QString& account, std::shared_ptr<ClientHandler> handler);
    void removeClient(const QString& account);
    std::shared_ptr<ClientHandler> getClient(const QString& account);

private:
    Ui::Server* ui;

    QMutex mutex;

    QSqlDatabase db;

    // QThreadPool* threadPool;
    bool listenFlag = false;
    QTcpServer* TCP;
};

#endif // SERVER_H
