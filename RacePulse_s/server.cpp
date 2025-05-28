#include "server.h"

#include <ClientHandler.h>

#include <QSqlQueryModel>

#include "qjsonobject.h"
#include "ui_server.h"

Server::Server(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Server)
{
    ui->setupUi(this);

    // logo 设置
    QIcon icon(":/images/logo.png");
    setWindowIcon(icon);
    setWindowTitle("赛搏");

    databaseConnect();
    on_pu_refresh_table_clicked();

    // 设置全局线程池
    // 避免多次创建 浪费系统资源
    // threadPool = QThreadPool::globalInstance();
    // int maxThreads = QThread::idealThreadCount() * 2;
    // threadPool->setMaxThreadCount(qMin(maxThreads, 300));
}

Server::~Server()
{
    // threadPool->waitForDone();
    delete ui;
}

bool Server::tcpListen()
{
    TCP = new QTcpServer(this);

    if (!TCP)
    {
        qDebug() << "TCP服务器创建失败";
        return false;
    }

    connect(TCP, &QTcpServer::newConnection, this, &Server::onNewConnection);

    // 添加错误处理
    connect(TCP, &QTcpServer::acceptError, this, [this](QAbstractSocket::SocketError error)
            { qDebug() << "接受连接错误:" << TCP->errorString(); });

    if (!TCP->listen(QHostAddress::Any, port))
    {
        qDebug() << "监听失败:" << TCP->errorString();
        return false;
    }
    qDebug() << "监听成功";
    return true;
}

void Server::onNewConnection()
{
    QTcpSocket* socket = TCP->nextPendingConnection();
    if (!socket)
    {
        qDebug() << "Error: Failed to get new connection";
        return;
    }

    ConnectionPool& pool = ConnectionPool::getInstance();

    // 创建 handler 并保存引用
    auto handler = std::make_shared<ClientHandler>(socket, pool, this);
    activeHandlers.append(handler);

    // 将 socket 移动到新线程
    QThread* thread = new QThread(this);
    socket->moveToThread(thread);
    handler->moveToThread(thread);

    // 在移动到新线程后建立连接
    connect(thread, &QThread::started, [handler, socket]()
            {
                // 在正确的线程上下文中建立连接
                connect(socket, &QTcpSocket::readyRead,
                        handler.get(), &ClientHandler::onReadyRead,
                        Qt::QueuedConnection);
                connect(socket, &QTcpSocket::disconnected,
                        handler.get(), &ClientHandler::onDisconnected,
                        Qt::QueuedConnection);
                connect(socket, &QTcpSocket::errorOccurred,
                        handler.get(), &ClientHandler::handleSocketError,
                        Qt::QueuedConnection); });

    // 处理清理
    connect(socket, &QTcpSocket::disconnected, [this, handler, thread, socket]()
            {
                // 从活动处理器列表中移除
                activeHandlers.removeOne(handler);

                // 清理线程和socket
                socket->deleteLater();
                thread->quit();
                thread->wait();
                thread->deleteLater(); });

    thread->start();
}
bool Server::databaseConnect()
{
    // 检查当前的数据库连接是否有效
    if (db.isOpen())
    {
        return true;
    }
    ConnectionPool& pool = ConnectionPool::getInstance();
    db = pool.getConnection();
    if (!db.isValid())
    {
        qDebug() << "clienthandler获取数据库连接失败";
        return false;
    }
    return true;
}

void Server::showtable(const QString& tablename)
{
    QString queryStr;

    if (tablename == "User")
    {
        queryStr = "SELECT usernum,nickname,gender,role,real_name,phone_number FROM User";
    }
    else if (tablename == "Contest")
    {
        queryStr = "SELECT contest_name,start_time,end_time,creator_id,description,status,contest_password FROM Contest";
    }

    QSqlQueryModel* model = new QSqlQueryModel(this);
    model->setQuery(queryStr, db);
    if (model->lastError().isValid())
    {
        qDebug() << "查询失败：" << model->lastError().text();
        return;
    }
    qDebug() << "加载行数：" << model->rowCount();
    ui->tableView->setModel(model);
}

void Server::addClient(const QString& account, std::shared_ptr<ClientHandler> handler)
{
    QMutexLocker locker(&mutex);
    clientsMap.insert(account, handler);
}

void Server::removeClient(const QString& account)
{
    QMutexLocker locker(&mutex);
    clientsMap.remove(account);
}

std::shared_ptr<ClientHandler> Server::getClient(const QString& account)
{
    QMutexLocker locker(&mutex);
    return clientsMap.value(account);
}

void Server::notifyClientsAndClose()
{
    QJsonObject shutdownMessage;
    shutdownMessage["tag"] = "client";
    shutdownMessage["type"] = "server_shutdown";
    shutdownMessage["message"] = "Server is shutting down";

    for (auto& handler : clientsMap)
    {
        if (handler)
        {
            handler->notifyClientShutdown(shutdownMessage);
        }
    }

    clientsMap.clear();
}

void Server::on_pu_listen_clicked()
{
    if (!listenFlag)
    {
        // 启动服务器逻辑保持不变
        if (tcpListen())
        {
            ui->pu_listen->setText("关闭服务器");
            ui->pu_listen->setStyleSheet(
                "QPushButton {"
                "    background-color: #4CAF50;" // 柔和的绿色
                "    color: white;"
                "    border: none;"
                "    border-radius: 4px;"
                "    padding: 8px;"
                "    font-size: 14px;"
                "}"
                "QPushButton:hover {"
                "    background-color: #45a049;" // 悬停时稍深的绿色
                "}"
                "QPushButton:pressed {"
                "    background-color: #3d8b40;" // 按下时更深的绿色
                "}");
            listenFlag = true;
            qDebug() << "服务器启动成功";
        }
        else
        {
            qDebug() << "服务器启动失败";
            return;
        }
    }
    else
    {
        ui->pu_listen->setStyleSheet(
            "QPushButton {"
            "    background-color: #f44336;" // 柔和的红色
            "    color: white;"
            "    border: none;"
            "    border-radius: 4px;"
            "    padding: 8px;"
            "    font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #e53935;" // 悬停时稍深的红色
            "}"
            "QPushButton:pressed {"
            "    background-color: #d32f2f;" // 按下时更深的红色
            "}");

        // 关闭服务器前通知客户端
        notifyClientsAndClose();

        // 逐一断开客户端连接
        for (auto& handler : clientsMap)
        {
            if (handler)
            {
                handler->onDisconnected();
            }
        }

        // 清空客户端映射
        clientsMap.clear();

        // 关闭服务器逻辑
        if (TCP)
        {
            TCP->close();
            qDebug() << "服务器已关闭";
        }

        ui->pu_listen->setText("开启服务器");
        listenFlag = false;
    }
}

void Server::on_pu_refresh_table_clicked()
{
    on_combo_table_currentIndexChanged(ui->combo_table->currentIndex());
}

void Server::on_combo_table_currentIndexChanged(int index)
{
    if (index == 0)
    {
        showtable("User");
    }
    else if (index == 1)
    {
        showtable("Contest");
    }
}
