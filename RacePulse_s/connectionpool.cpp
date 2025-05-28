#include "connectionpool.h"

#include <QSqlQuery>

ConnectionPool& ConnectionPool::getInstance()
{
    static ConnectionPool instance; // 确保是同一个实例
    return instance;
}

QSqlDatabase ConnectionPool::getConnection()
{
    QMutexLocker locker(&mutex);

    // 尝试获取现有连接并验证
    for (int i = 0; i < pool.size(); ++i)
    {
        QSqlDatabase db = pool.dequeue();
        if (validateConnection(db))
        {
            return db;
        }
        // 无效连接则关闭
        db.close();
    }

    // 创建新连接的逻辑保持不变
    if (pool.size() < maxConnections)
    {
        QString connectionName = QString("Connection_%1").arg(connectionCounter++);
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", connectionName);
        db.setHostName(dbHost);
        db.setPort(3306);
        db.setDatabaseName(dbName);
        db.setUserName(dbUser);
        db.setPassword(dbPassword);

        if (db.open())
        {
            return db;
        }
    }

    qWarning() << "数据库连接池已满，无法创建新连接";
    return QSqlDatabase();
}

ConnectionPool::ConnectionPool()
    : maxConnections(310)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", dbName);
    db.setHostName("127.0.0.1"); // 或者你的MySQL服务器地址
    db.setPort(3306);
    db.setDatabaseName(dbName); // 你的数据库名
    db.setUserName("root");     // 你的数据库用户名
    db.setPassword("2003");     // 你的数据库密码

    if (!db.open())
    {
        qDebug() << "打开数据库失败" << db.lastError().text();
    }
    else
    {
        QSqlQuery query(db);
        // 创建用户表
        query.exec(
            "CREATE TABLE User ("
            "user_id INT AUTO_INCREMENT PRIMARY KEY,"
            "usernum VARCHAR(50) UNIQUE NOT NULL,"
            "password VARCHAR(100) NOT NULL,"
            "nickname VARCHAR(50) DEFAULT NULL,"
            "avatar VARCHAR(255) DEFAULT NULL,"
            "gender ENUM('男', '女', '保密') DEFAULT '保密',"
            "face_path VARCHAR(255) DEFAULT NULL,"
            "role ENUM('管理员', '裁判', '参赛者') NOT NULL,"
            "real_name VARCHAR(50),"
            "phone_number VARCHAR(20)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;");
        if (query.lastError().isValid())
        {
            qDebug() << "创建用户表失败：" << query.lastError().text();
        }
        // 创建赛事表
        query.exec(
            "CREATE TABLE contest ("
            "contest_id VARCHAR(20) PRIMARY KEY,"
            "contest_name VARCHAR(100) NOT NULL,"
            "contest_logo VARCHAR(255) DEFAULT NULL,"
            "start_time DATETIME,"
            "end_time DATETIME,"
            "creator_id INT,"
            "description TEXT,"
            "status ENUM('未开始', '进行中', '已结束') DEFAULT '未开始',"
            "contest_password VARCHAR(255) DEFAULT NULL COMMENT '比赛密码，非必填',"
            "FOREIGN KEY (creator_id) REFERENCES User(user_id)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;");
        if (query.lastError().isValid())
        {
            qDebug() << "创建赛事表失败：" << query.lastError().text();
        }
        // 创建参赛人员表
        query.exec(
            "CREATE TABLE participant ("
            "participant_id INT AUTO_INCREMENT PRIMARY KEY,"
            "user_id INT,"
            "contest_id INT,"
            "sign_in_time DATETIME,"
            "sign_in_status ENUM('未签到', '已签到') DEFAULT '未签到',"
            "team_name VARCHAR(50),"
            "FOREIGN KEY (user_id) REFERENCES user(user_id),"
            "FOREIGN KEY (contest_id) REFERENCES contest(contest_id)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;");
        if (query.lastError().isValid())
        {
            qDebug() << "创建参赛人员表失败：" << query.lastError().text();
        }
        // 创建成绩表
        query.exec(
            "CREATE TABLE score ("
            "score_id INT AUTO_INCREMENT PRIMARY KEY,"
            "participant_id INT,"
            "contest_id INT,"
            "start_time DATETIME,"
            "end_time DATETIME,"
            "total_score DECIMAL(10,2),"
            "FOREIGN KEY (participant_id) REFERENCES participant(participant_id),"
            "FOREIGN KEY (contest_id) REFERENCES contest(contest_id)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;");
        if (query.lastError().isValid())
        {
            qDebug() << "创建成绩表失败：" << query.lastError().text();
        }
        // 创建签到日志表
        query.exec(
            "CREATE TABLE sign_in_log ("
            "log_id INT AUTO_INCREMENT PRIMARY KEY,"
            "participant_id INT,"
            "sign_in_time DATETIME,"
            "sign_in_type ENUM('正常', '迟到') DEFAULT '正常',"
            "FOREIGN KEY (participant_id) REFERENCES participant(participant_id)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;");
        if (query.lastError().isValid())
        {
            qDebug() << "创建签到日志表失败：" << query.lastError().text();
        }
    }
}

ConnectionPool::~ConnectionPool()
{
    while (!pool.isEmpty())
    {
        QSqlDatabase db = pool.dequeue();
        db.close();
    }
}

void ConnectionPool::releaseConnection(QSqlDatabase& db)
{
    QMutexLocker locker(&mutex);
    if (pool.size() < maxConnections)
    {
        pool.enqueue(db);
    }
    else
    {
        db.close();
    }
}

void ConnectionPool::setMaxConnections(int max) // 设置最大连接数
{
    QMutexLocker locker(&mutex);
    maxConnections = max;
}

int ConnectionPool::getMaxConnections() const // 获取当前最大连接数
{
    return maxConnections;
}

bool ConnectionPool::validateConnection(QSqlDatabase& db)
{
    if (!db.isOpen())
    {
        return db.open();
    }

    QSqlQuery testQuery(db);
    return testQuery.exec("SELECT 1");
}
