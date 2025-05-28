#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <QSqlDatabase.h>

#include <QMutex>
#include <QQueue>
#include <QSettings>
#include <QSqlError>
#include <QVector>

#include "qdebug.h"

class ConnectionPool
{
private:
    ConnectionPool();
    ConnectionPool(const ConnectionPool&) = delete;            // 删除复制构造函数
    ConnectionPool& operator=(const ConnectionPool&) = delete; // 删除赋值操作符
    ~ConnectionPool();

public:
    static ConnectionPool& getInstance();
    QSqlDatabase getConnection();

    void releaseConnection(QSqlDatabase& db);

    void setMaxConnections(int max);
    int getMaxConnections() const;

private:
    // 添加连接验证方法
    bool validateConnection(QSqlDatabase& db);

private:
    // 使用配置文件或环境变量
    QString dbHost = QSettings().value("db/host", "127.0.0.1").toString();
    QString dbName = QSettings().value("db/name", "RacePulse_server").toString();
    QString dbUser = QSettings().value("db/user", "root").toString();
    QString dbPassword = QSettings().value("db/password", "2003").toString();

    QMutex mutex;
    QQueue<QSqlDatabase> pool;
    int maxConnections; // 最大连接数
    int connectionCounter = 0;
};

#endif // CONNECTIONPOOL_H
