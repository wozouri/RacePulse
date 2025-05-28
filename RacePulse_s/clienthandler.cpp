#include "clienthandler.h"

#include <qbuffer.h>
#include <qmessagebox.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "qsqlquery.h"
#include "server.h"

ClientHandler::ClientHandler(QTcpSocket* socket, ConnectionPool& pool, Server* srv)
    : m_socket(socket), srv(srv), pool(pool)
{
    if (!m_socket)
    {
        throw std::runtime_error("Null socket provided to ClientHandler");
    }

    // 设置socket父对象，确保正确的资源管理
    m_socket->setParent(this);

    // 加载 Haar 分类器
    if (!faceCascade.load("D:/Lib/OpenCV-MinGW-Build-OpenCV-4.5.5-x64/etc/haarcascades/haarcascade_frontalface_default.xml"))
    {
        QMessageBox::critical(nullptr, "Error", "Failed to load Haar Cascade.");
        return;
    }

    // 初始化心跳定时器
    heartbeatTimer = new QTimer(this);
    connect(heartbeatTimer, &QTimer::timeout, this, [this]()
            {
        if (++missedHeartbeats >= MAX_MISSED_HEARTBEATS) {
            qDebug() << "Client" << account << "heart beat timeout, disconnecting...";
            m_socket->disconnectFromHost();
        } else {
            // 发送心跳包
            QJsonObject heartbeat;
            heartbeat["tag"] = "heartbeat";
            sendJsonResponse(heartbeat);
        } });

    heartbeatTimer->start(HEARTBEAT_INTERVAL);

    connect(this, &ClientHandler::dataReceived, this, &ClientHandler::processRequest);

    databasesConnect();
}

ClientHandler::~ClientHandler()
{
    m_socket = nullptr;
    if (db.isOpen())
    {
        db = QSqlDatabase();
    }
}

bool ClientHandler::databasesConnect()
{
    // 检查当前的数据库连接是否有效
    if (db.isOpen())
    {
        return true;
    }
    db = pool.getConnection();
    if (!db.isValid())
    {
        qDebug() << "clienthandler获取数据库连接失败";
        return false;
    }
    return true;
}

void ClientHandler::handleSocketError(QAbstractSocket::SocketError error)
{
    QString errorMessage = m_socket->errorString();
    qWarning() << "Socket error occurred for client" << account << ":" << errorMessage;

    // 根据错误类型采取不同的处理措施
    switch (error)
    {
    case QAbstractSocket::RemoteHostClosedError:
        // 远程主机正常关闭，不需要特殊处理
        break;

    case QAbstractSocket::SocketTimeoutError:
        // 超时错误，可以尝试重新连接
        m_socket->disconnectFromHost();
        m_socket->connectToHost(m_socket->peerAddress(), m_socket->peerPort());
        break;

    default:
        // 其他错误，直接断开连接
        m_socket->disconnectFromHost();
        break;
    }
}

void ClientHandler::onReadyRead()
{
    // 从套接字中读取所有可用数据并追加到缓存区
    buffer += m_socket->readAll();

    while (true)
    {
        int endIndex = buffer.indexOf("\n}\nEND"); // 6byte 只搜END可能错误截断，不过这种搜索方式还是不安全
        if (endIndex == -1)
        {
            break; // 未找到完整消息，等待更多数据
        }

        // 打印接收到的数据长度
        if (buffer.size() > 500)
            qDebug() << m_socket->socketDescriptor() << ">   Received data (size):" << buffer.size();
        else
            qDebug() << m_socket->socketDescriptor() << ">   Received data (content):" << buffer;

        // 提取完整消息
        QByteArray completeMessage = buffer.left(endIndex + 3);
        buffer.remove(0, endIndex + 6); // 移除已处理的消息

        // 解析 JSON 数据
        try
        {
            QJsonDocument jsonDoc = QJsonDocument::fromJson(completeMessage);
            if (!jsonDoc.isNull() && jsonDoc.isObject())
            {
                QJsonObject jsonObj = jsonDoc.object();
                if (jsonObj["tag"] == "heartbeat")
                {
                    missedHeartbeats = 0;
                }
                else
                {
                    emit dataReceived(jsonObj); // 发出信号以处理数据
                }
            }
            else
            {
                throw std::runtime_error("Invalid JSON data");
            }
        }
        catch (const std::exception& e)
        {
            qCritical() << "Processing error:" << e.what();
        }
    }

    // 如果缓存区中仍有数据但未处理，等待下一次读取
}

void ClientHandler::onDisconnected()
{
    qDebug() << "客户端断开连接";

    // 如果账户不是默认值，执行移除客户端操作
    if (account != "0" && !account.isEmpty())
    {
        srv->removeClient(account);
    }

    // 关闭数据库连接
    if (db.isOpen())
    {
        db = QSqlDatabase();
        qDebug() << "数据库连接已关闭";
    }

    m_socket = nullptr;
}

void ClientHandler::closeConnection()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
    {
        m_socket->disconnectFromHost();
        if (m_socket->state() == QAbstractSocket::UnconnectedState)
        {
            m_socket = nullptr;
        }
    }
}

void ClientHandler::processRequest(const QJsonObject& jsonObj)
{
    // 确保数据库连接
    if (!databasesConnect())
        return;

    QString tag = jsonObj["tag"].toString();

    if (tag == "login")
    {
        dealLogin(jsonObj);
    }
    else if (tag == "register")
    {
        dealRegister(jsonObj);
    }
    else if (tag == "home")
    {
        if (jsonObj["mode"] == "search_term")
            dealSearchTerm(jsonObj);
        else if (jsonObj["mode"] == "face_bind")
            dealContainsFace(jsonObj);
    }
    else if (tag == "face")
    {
        if (jsonObj["mode"] == "check")
        {
            dealCheckFace(jsonObj);
        }
        else if (jsonObj["mode"] == "save" || jsonObj["mode"] == "modify")
        {
            dealUpdateFace(jsonObj);
        }
    }

    // 确保在处理完请求后释放数据库连接
    pool.releaseConnection(db);
}

void ClientHandler::receiveMessage(const QJsonObject& json) // 收到别的客户端发送的消息 然后转发
{
    if (json["tag"] == "duplicate_logins")
    { // 把在线用户挤下线
        forwordKickedOffline(json);
    }
}

void ClientHandler::notifyClientShutdown(const QJsonObject& json)
{
    QMutexLocker locker(&socketMutex);

    json["tag"] = "shutdown";
    if (m_socket && m_socket->isOpen())
    {
        sendJsonResponse(json);
    }
}

void ClientHandler::sendJsonResponse(const QJsonObject& responseJson)
{
    QMutexLocker locker(&socketMutex);

    // 将消息转换为 JSON 格式并添加到消息队列
    QByteArray jsonData = QJsonDocument(responseJson).toJson() + "END";
    messageQueue.enqueue(jsonData);

    // 处理消息队列
    processMessageQueue();
}

void ClientHandler::processMessageQueue()
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
        qDebug() << "Message sent successfully:" << message.size() << " > " << m_socket->socketDescriptor();
    else
        qDebug() << "Message sent successfully:" << message << " > " << m_socket->socketDescriptor();

    // 标记发送完成，继续处理队列中的下一条消息
    isSending = false;
    processMessageQueue();
}

void ClientHandler::sendErrorResponse(QJsonObject qjsonObj, const QString& reason)
{
    qjsonObj["result"] = "fail";
    qjsonObj["reason"] = reason;
    sendJsonResponse(qjsonObj);
}

void ClientHandler::dealLogin(const QJsonObject& json)
{
    QMutexLocker locker(&dbMutex);
    QJsonObject response;
    response["tag"] = "login";

    // 验证输入
    if (!json.contains("usernum") || !json.contains("password"))
    {
        sendErrorResponse(response, "缺少登录信息");
        return;
    }

    const QString usernum = json["usernum"].toString();
    const QString password = json["password"].toString();

    if (usernum.isEmpty() || password.isEmpty())
    {
        sendErrorResponse(response, "账号或密码不能为空");
        return;
    }

    // 查询用户信息
    QSqlQuery qry(db);
    qry.prepare(
        "SELECT password, nickname, avatar, role FROM user "
        "WHERE usernum = :usernum");
    qry.bindValue(":usernum", usernum);

    if (!qry.exec())
    {
        sendErrorResponse(response, "数据库查询错误");
        qDebug() << "数据库查询错误:" << qry.lastError().text();
        return;
    }

    // 验证用户存在性和密码
    if (!qry.next())
    {
        sendErrorResponse(response, "无效的用户名或密码"); // 用户不存在
        return;
    }

    const QString correctPassword = qry.value("password").toString();
    if (password != correctPassword)
    {
        sendErrorResponse(response, "无效的用户名或密码"); // 密码错误
        return;
    }

    // 处理重复登录
    auto existingClient = srv->getClient(usernum);
    if (existingClient)
    {
        QJsonObject kickMsg;
        kickMsg["reason"] = "您的账号在其他地方登录，当前会话已断开。";
        existingClient->notifyClientShutdown(kickMsg);
        existingClient->closeConnection();
    }

    // 获取用户信息
    const QString nickname = qry.value("nickname").toString();
    const QString avatarFilename = qry.value("avatar").toString();
    const QString role = qry.value("role").toString();

    // 读取头像数据
    QString avatarBase64 = loadAvatarAsBase64(avatarFilename);

    // 构建成功响应
    response["result"] = "success";
    response["usernum"] = usernum;
    response["nickname"] = nickname;
    response["role"] = role;
    response["avatar_data"] = avatarBase64;

    sendJsonResponse(response);
    qDebug() << "用户" << usernum << "登录成功";
}

// 将头像文件转换为Base64
QString ClientHandler::loadAvatarAsBase64(const QString& avatarFilename)
{
    // 构建头像文件路径
    QString avatarPath = QString("./avatar/%1").arg(avatarFilename);

    // 检查是否存在有效的头像文件
    QFile file(avatarPath);
    if (!file.exists())
    {
        // 如果文件不存在，使用默认头像
        avatarPath = "./avatar/default.png";
        file.setFileName(avatarPath);
    }

    // 读取头像文件
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "无法打开头像文件:" << avatarPath;
        return QString();
    }

    // 读取文件数据并转换为Base64
    QByteArray imageData = file.readAll();
    file.close();

    // 验证图片数据有效性
    QImage image;
    if (!image.loadFromData(imageData))
    {
        qDebug() << "无效的图片数据:" << avatarPath;
        return QString();
    }

    // 如果图片过大，进行压缩
    if (imageData.size() > 1024 * 1024)
    { // 如果大于1MB
        QBuffer tp_buffer;
        tp_buffer.open(QIODevice::WriteOnly);
        image.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation)
            .save(&tp_buffer, "PNG", 80);
        imageData = tp_buffer.data();
    }

    return QString(imageData.toBase64());
}

void ClientHandler::dealRegister(const QJsonObject& json)
{
    QMutexLocker locker(&dbMutex);
    QJsonObject response;
    response["tag"] = "register";

    // 数据库连接检查
    if (!db.isOpen())
    {
        sendErrorResponse(response, "数据库连接失败");
        return;
    }

    // 检查必要字段
    if (!json.contains("nickname") || !json.contains("password"))
    {
        sendErrorResponse(response, "缺少必要注册信息");
        return;
    }

    const QString nickname = json["nickname"].toString();
    const QString password = json["password"].toString();

    // 验证输入
    if (nickname.isEmpty() || password.isEmpty())
    {
        sendErrorResponse(response, "昵称或密码不能为空");
        return;
    }

    // 昵称检查
    if (json["nickname_ischeck"] != "true")
    {
        if (!checkNicknameAvailable(nickname))
        {
            sendErrorResponse(response, "昵称已被使用");
            return;
        }
    }

    // 生成并验证唯一账号
    QString usernum = generateUniqueUsernum();
    if (usernum.isEmpty())
    {
        sendErrorResponse(response, "生成账号失败");
        return;
    }

    // 处理头像
    QString avatarPath = handleAvatar(usernum, json);
    if (avatarPath.isEmpty())
    {
        avatarPath = "default.png"; // 使用默认头像
    }

    // 开始事务
    if (!db.transaction())
    {
        sendErrorResponse(response, "开始事务失败: " + db.lastError().text());
        return;
    }

    // 插入用户记录
    if (!insertUserRecord(usernum, password, nickname, avatarPath))
    {
        db.rollback();
        sendErrorResponse(response, "注册失败: " + db.lastError().text());
        return;
    }

    // 提交事务
    if (!db.commit())
    {
        db.rollback();
        sendErrorResponse(response, "提交事务失败: " + db.lastError().text());
        return;
    }

    // 发送成功响应
    response["result"] = "success";
    response["usernum"] = usernum;
    response["avatar"] = avatarPath;
    sendJsonResponse(response);
    qDebug() << "用户" << usernum << "注册成功";
}

// 检查昵称是否可用
bool ClientHandler::checkNicknameAvailable(const QString& nickname)
{
    QSqlQuery qry(db);
    qry.prepare("SELECT COUNT(*) FROM User WHERE nickname = ?");
    qry.addBindValue(nickname);

    if (qry.exec() && qry.next())
    {
        return qry.value(0).toInt() == 0;
    }
    return false;
}

// 生成唯一用户账号
QString ClientHandler::generateUniqueUsernum()
{
    QSqlQuery qry(db);
    const int MAX_ATTEMPTS = 10;

    for (int i = 0; i < MAX_ATTEMPTS; ++i)
    {
        // 生成9位随机数
        QString usernum = QString::number(
            QRandomGenerator::global()->bounded(100000000, 1000000000));

        qry.prepare("SELECT COUNT(*) FROM User WHERE usernum = ?");
        qry.addBindValue(usernum);

        if (qry.exec() && qry.next() && qry.value(0).toInt() == 0)
        {
            return usernum;
        }
    }
    return QString();
}

// 处理头像上传
QString ClientHandler::handleAvatar(const QString& usernum, const QJsonObject& json)
{
    // 检查是否有头像数据
    if (!json.contains("avatar_data"))
    {
        return QString();
    }

    // 创建头像目录
    QDir avatarDir("./avatar");
    if (!avatarDir.exists())
    {
        avatarDir.mkpath(".");
    }

    QString avatarPath = QString("./avatar/%1.png").arg(usernum);

    // 解码Base64头像数据
    QByteArray imageData = QByteArray::fromBase64(
        json["avatar_data"].toString().toUtf8());

    if (imageData.isEmpty())
    {
        return QString();
    }

    // 保存头像文件
    QImage image;
    if (!image.loadFromData(imageData))
    {
        return QString();
    }

    // 压缩并保存头像
    QImage scaled = image.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (!scaled.save(avatarPath, "PNG", 80))
    {
        return QString();
    }

    return QString("%1.png").arg(usernum);
}

// 插入用户记录
bool ClientHandler::insertUserRecord(const QString& usernum,
                                     const QString& password,
                                     const QString& nickname,
                                     const QString& avatar)
{
    QSqlQuery qry(db);
    qry.prepare(
        "INSERT INTO User (usernum, password, nickname, avatar, role) "
        "VALUES (?, ?, ?, ?, '参赛者')");

    qry.addBindValue(usernum);
    qry.addBindValue(password);
    qry.addBindValue(nickname);
    qry.addBindValue(avatar);

    return qry.exec();
}

void ClientHandler::dealSearchTerm(const QJsonObject& json)
{
    QMutexLocker locker(&dbMutex);

    // 检查JSON对象是否有效
    if (json.isEmpty())
    {
        QJsonObject errorResponse;
        errorResponse["error"] = "Invalid empty JSON request";
        errorResponse["tag"] = "home";
        errorResponse["mode"] = "search_term";
        sendJsonResponse(errorResponse);
        return;
    }

    // 检查数据库连接
    if (!db.isOpen())
    {
        qCritical() << "Database is not open!";
        QJsonObject errorResponse;
        errorResponse["error"] = "Database connection error";
        errorResponse["tag"] = "home";
        errorResponse["mode"] = "search_term";
        sendJsonResponse(errorResponse);
        return;
    }

    try
    {
        // 提取并验证搜索参数
        QString searchTerm = json.value("search_term").toString().trimmed();
        // if (searchTerm.isEmpty())
        // {
        //     QJsonObject errorResponse;
        //     errorResponse["error"] = "Search term cannot be empty";
        //     errorResponse["tag"] = "home";
        //     errorResponse["mode"] = "search_term";
        //     sendJsonResponse(errorResponse);
        //     return;
        // }

        // 限制搜索词长度
        if (searchTerm.length() > 100)
        {
            searchTerm = searchTerm.left(100);
        }

        int pageNum = qMax(1, json.value("page_num").toInt(1));           // 确保页码至少为1
        int pageSize = qBound(1, json.value("page_size").toInt(10), 100); // 限制每页大小在1-100之间

        // 检查搜索词是否为数字
        bool isNumeric = false;
        searchTerm.toInt(&isNumeric);

        // 计算偏移量
        int offset = (pageNum - 1) * pageSize;

        // 准备主查询
        QSqlQuery qry(db);
        QString searchQuery =
            "SELECT contest_id, contest_name, contest_logo, "
            "start_time, end_time, creator_id, description, status, contest_password, "
            "CASE "
            "  WHEN contest_name = :exact_match THEN 1 "
            "  WHEN contest_name LIKE :starts_with THEN 2 "
            "  WHEN contest_name LIKE :contains THEN 3 "
            "  ELSE 4 "
            "END AS relevance_score "
            "FROM contest "
            "WHERE contest_name LIKE :contains "
            "   OR (:is_numeric = 1 AND contest_id = CAST(:search_id AS UNSIGNED)) "
            "ORDER BY relevance_score, start_time DESC, contest_id "
            "LIMIT :page_size OFFSET :offset";

        qry.prepare(searchQuery);

        // 绑定查询参数
        qry.bindValue(":exact_match", searchTerm);
        qry.bindValue(":starts_with", searchTerm + "%");
        qry.bindValue(":contains", "%" + searchTerm + "%");
        qry.bindValue(":is_numeric", isNumeric);
        qry.bindValue(":search_id", searchTerm.toInt());
        qry.bindValue(":page_size", pageSize);
        qry.bindValue(":offset", offset);

        // 执行查询并检查错误
        if (!qry.exec())
        {
            qCritical() << "Search query error:" << qry.lastError().text();
            QJsonObject errorResponse;
            errorResponse["error"] = "Database query failed: " + qry.lastError().text();
            errorResponse["tag"] = "home";
            errorResponse["mode"] = "search_term";
            sendJsonResponse(errorResponse);
            return;
        }

        // 构建结果数组
        QJsonArray resultArray;
        while (qry.next())
        {
            QJsonObject contestObj;
            contestObj["contest_id"] = qry.value("contest_id").toString();
            contestObj["contest_name"] = qry.value("contest_name").toString();
            contestObj["contest_logo"] = qry.value("contest_logo").toString();
            contestObj["start_time"] = qry.value("start_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            contestObj["end_time"] = qry.value("end_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");

            // 获取创建者昵称
            int creatorId = qry.value("creator_id").toInt();
            QSqlQuery creatorQry(db);
            creatorQry.prepare("SELECT nickname FROM User WHERE user_id = :creator_id");
            creatorQry.bindValue(":creator_id", creatorId);

            QString creatorName = "Unknown"; // 默认值
            if (creatorQry.exec() && creatorQry.next())
            {
                creatorName = creatorQry.value("nickname").toString();
            }
            contestObj["creator_nickname"] = creatorName;

            contestObj["description"] = qry.value("description").toString();
            contestObj["status"] = qry.value("status").toString();
            contestObj["contest_password"] = qry.value("contest_password").toString();

            resultArray.append(contestObj);
        }

        // 查询总记录数
        QSqlQuery countQry(db);
        QString countQuery =
            "SELECT COUNT(*) as total_count "
            "FROM contest "
            "WHERE contest_name LIKE :contains "
            "   OR (:is_numeric = 1 AND contest_id = CAST(:search_id AS UNSIGNED))";

        countQry.prepare(countQuery);
        countQry.bindValue(":contains", "%" + searchTerm + "%");
        countQry.bindValue(":is_numeric", isNumeric);
        countQry.bindValue(":search_id", searchTerm.toInt());

        int totalCount = 0;
        if (countQry.exec() && countQry.next())
        {
            totalCount = countQry.value("total_count").toInt();
        }
        else
        {
            qWarning() << "Count query failed:" << countQry.lastError().text();
        }

        // 构建响应
        QJsonObject responseJson;
        responseJson["search_term"] = searchTerm;
        responseJson["page_num"] = pageNum;
        responseJson["page_size"] = pageSize;
        responseJson["total_count"] = totalCount;
        responseJson["total_pages"] = qCeil(static_cast<double>(totalCount) / pageSize);
        responseJson["results"] = resultArray;
        responseJson["tag"] = "home";
        responseJson["mode"] = "search_term";

        // 发送响应
        sendJsonResponse(responseJson);

        qDebug() << "Contest search completed. Search term:" << searchTerm
                 << "Page:" << pageNum
                 << "Page size:" << pageSize
                 << "Total results:" << totalCount;
    }
    catch (const std::exception& e)
    {
        qCritical() << "Error in dealSearchTerm:" << e.what();
        QJsonObject errorResponse;
        errorResponse["error"] = QString("Internal server error: %1").arg(e.what());
        errorResponse["tag"] = "home";
        errorResponse["mode"] = "search_term";
        sendJsonResponse(errorResponse);
    }
}
void ClientHandler::dealContainsFace(const QJsonObject& json)
{
    QMutexLocker locker(&dbMutex);
    QJsonObject qjsonObj;
    QSqlQuery qry(db);

    qjsonObj["tag"] = "home";
    qjsonObj["mode"] = "face_bind";

    QString usernum = json["usernum"].toString();
    QString face_path = nullptr;

    // 通过用户账号查找面部数据地址
    qry.prepare("SELECT face_path FROM User WHERE usernum =:usernum");
    qry.bindValue(":usernum", usernum);

    if (qry.exec() && qry.next())
    {
        face_path = qry.value("face_path").toString();
        if (face_path.isEmpty())
        {
            qjsonObj["status"] = "no";
            qjsonObj["reason"] = "账号未上传认证照片";
        }
        else
        {
            // 打开文件并检查是否存在以 "feature_" 开头的数据
            cv::FileStorage fs(face_path.toStdString(), cv::FileStorage::READ);
            if (!fs.isOpened())
            {
                qjsonObj["status"] = "no";
                qjsonObj["reason"] = "无法打开面部数据文件";
                qDebug() << "Failed to open file:" << face_path;
            }
            else
            {
                bool containsFeature = false;
                cv::FileNode rootNode = fs.root();
                for (cv::FileNodeIterator it = rootNode.begin(); it != rootNode.end(); ++it)
                {
                    std::string nodeName = (*it).name();
                    if (nodeName.find("feature_") == 0) // 检查节点名称是否以 "feature_" 开头
                    {
                        containsFeature = true;
                        break;
                    }
                }

                if (containsFeature)
                {
                    qjsonObj["status"] = "yes";
                    qjsonObj["reason"] = "面部数据已绑定";
                }
                else
                {
                    qjsonObj["status"] = "no";
                    qjsonObj["reason"] = "面部数据文件为空或格式错误";
                }

                fs.release(); // 关闭文件
            }
        }
    }
    else
    {
        qjsonObj["status"] = "no";
        qjsonObj["reason"] = "数据库查询出错";
        qDebug() << "Query failed:" << qry.lastError().text();
    }

    // 发送响应
    sendJsonResponse(qjsonObj);
}

void ClientHandler::dealCheckFace(const QJsonObject& json)
{
    QMutexLocker locker(&dbMutex);
    QJsonObject qjsonObj;
    QSqlQuery qry(db);

    qjsonObj["tag"] = "face";
    qjsonObj["mode"] = "check";

    QString usernum = json["usernum"].toString();
    QString face_path = ""; // 初始化为空字符串

    // 通过用户账号查找面部数据地址
    qry.prepare("SELECT face_path FROM User WHERE usernum = :usernum");
    qry.bindValue(":usernum", usernum);

    if (!qry.exec() || !qry.next()) // 查询失败时直接返回
    {
        sendErrorResponse(qjsonObj, "数据库查询出错");
        qDebug() << "Query failed:" << qry.lastError().text();
        return;
    }

    face_path = qry.value("face_path").toString();

    if (face_path.isEmpty()) // 如果 face_path 为空，返回错误信息
    {
        sendErrorResponse(qjsonObj, "账号未上传认证照片");
        return;
    }

    // 提取 Base64 字符串并解码为字节数组
    QByteArray base64Data = json["face"].toString().toUtf8();
    QByteArray imageData = QByteArray::fromBase64(base64Data);

    // 将字节数组转换为 QImage
    QImage image;
    if (!image.loadFromData(imageData, "PNG")) // 假设发送的是 PNG 格式
    {
        sendErrorResponse(qjsonObj, "Error: Failed to load image from data");
        return;
    }

    // 图像处理与人脸验证逻辑
    cv::Mat matImage = QImageToCvMat(image);
    if (matImage.empty())
    {
        sendErrorResponse(qjsonObj, "Error: Failed to convert QImage to cv::Mat");
        return;
    }

    cv::Mat grayImage;
    cv::cvtColor(matImage, grayImage, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(grayImage, grayImage);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(grayImage, faces, 1.1, 3, 0, cv::Size(30, 30));

    if (faces.empty()) // 如果没有检测到人脸，返回错误信息
    {
        sendErrorResponse(qjsonObj, "未找到人脸，请正视摄像头");
        return;
    }
    if (faces.size() > 1) // 如果检测到多张人脸，返回错误信息
    {
        sendErrorResponse(qjsonObj, "检测到多张人脸，请确保仅有一人");
        return;
    }

    // 提取特征向量并进行身份验证
    cv::Mat faceROI = matImage(faces[0]);
    cv::Mat resizedFace;
    cv::resize(faceROI, resizedFace, cv::Size(112, 112));

    cv::Mat featureVector = extractFeatureVector(resizedFace);

    bool isVerified = verifyIdentity(featureVector, face_path.toStdString());

    if (isVerified)
    {
        qjsonObj["result"] = "success";
    }
    else
    {
        qjsonObj["result"] = "fail";
        qjsonObj["reason"] = "人脸认证未通过";
    }

    sendJsonResponse(qjsonObj);
}

void ClientHandler::dealUpdateFace(const QJsonObject& json)
{
    if (!json.contains("usernum") || !json.contains("face") || json["face"].toString().isEmpty())
    {
        sendErrorResponse(QJsonObject(), "Face data is missing or empty.");
        return;
    }

    QJsonObject qjsonObj;
    qjsonObj["tag"] = "face";
    qjsonObj["mode"] = json["mode"];

    QString usernum = json["usernum"].toString();
    QString relativePath = "./faces/" + usernum + ".yml";

    // 检查 mode 是否为 modify
    QString mode = json["mode"].toString();
    if (mode == "modify")
    {
        QFile file(relativePath);
        if (file.exists())
        {
            if (!file.remove())
            {
                sendErrorResponse(qjsonObj, "Failed to delete existing feature vector file.");
                return;
            }
            qDebug() << "Existing feature vector file deleted for usernum:" << usernum;
        }
    }

    // 创建文件夹
    QDir dir("./faces");
    if (!dir.exists() && !dir.mkpath("."))
    {
        sendErrorResponse(qjsonObj, "Failed to create directory: ./faces");
        return;
    }

    // 数据库更新
    {
        QMutexLocker locker(&dbMutex);
        db.transaction();
        QSqlQuery qry(db);
        qry.prepare("UPDATE User SET face_path = :face_path WHERE usernum = :usernum");
        qry.bindValue(":face_path", relativePath);
        qry.bindValue(":usernum", usernum);
        if (!qry.exec())
        {
            db.rollback();
            sendErrorResponse(qjsonObj, "Failed to update face_path: " + qry.lastError().text());
            return;
        }
        db.commit();
    }

    // 图像解码
    QByteArray imageData = QByteArray::fromBase64(json["face"].toString().toUtf8());
    QImage image;
    if (!image.loadFromData(imageData, "PNG"))
    {
        sendErrorResponse(qjsonObj, "Failed to load image from data.");
        return;
    }

    cv::Mat matImage = QImageToCvMat(image);
    if (matImage.empty())
    {
        sendErrorResponse(qjsonObj, "Failed to convert QImage to cv::Mat.");
        return;
    }

    // 人脸检测
    cv::Mat grayImage;
    cv::cvtColor(matImage, grayImage, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(grayImage, grayImage);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(grayImage, faces, 1.1, 3, 0, cv::Size(30, 30));

    if (faces.empty())
    {
        sendErrorResponse(qjsonObj, "未找到人脸，请正视摄像头");
        return;
    }

    if (faces.size() > 1)
    {
        sendErrorResponse(qjsonObj, "检测到多张人脸，请确保仅有一人");
        return;
    }

    // 提取和保存特征向量
    cv::Mat faceROI = matImage(faces[0]);
    cv::Mat resizedFace;
    cv::resize(faceROI, resizedFace, cv::Size(112, 112));

    cv::Mat featureVector = extractFeatureVector(resizedFace);
    if (!saveFeatureVector(featureVector, relativePath.toStdString()))
    {
        sendErrorResponse(qjsonObj, "保存特征向量失败");
        return;
    }

    qjsonObj["result"] = "success";
    sendJsonResponse(qjsonObj);
    qDebug() << "Face data updated successfully for usernum:" << usernum;
}

bool ClientHandler::saveFeatureVector(const cv::Mat& featureVector, const std::string& filename)
{
    cv::FileStorage fs(filename, cv::FileStorage::APPEND);
    if (!fs.isOpened())
    {
        qDebug() << "Failed to open file for saving features: " << QString::fromStdString(filename);
        return false; // 文件打开失败，返回 false
    }

    // 生成时间戳
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string featureName = "feature_" + std::to_string(timestamp);

    // 保存特征向量
    fs << featureName << featureVector;

    fs.release();
    qDebug() << "Feature vector saved to: " << QString::fromStdString(filename)
             << " with name: " << QString::fromStdString(featureName);

    return true; // 成功保存，返回 true
}

bool ClientHandler::verifyIdentity(const cv::Mat& inputFeature, const std::string& filename)
{
    qDebug() << "Input feature size:" << inputFeature.size().width << " " << inputFeature.size().height
             << "channels:" << inputFeature.channels()
             << "type:" << inputFeature.type();

    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        qDebug() << "Failed to open file for reading features!";
        fs.release();
        return false;
    }

    // 转换输入特征到float类型
    cv::Mat normalizedInputFeature;
    if (inputFeature.type() != CV_32FC1)
    {
        inputFeature.convertTo(normalizedInputFeature, CV_32FC1);
    }
    else
    {
        normalizedInputFeature = inputFeature.clone();
    }

    // 对输入特征进行L2归一化
    normalize(normalizedInputFeature, normalizedInputFeature, 1.0, 0.0, cv::NORM_L2);

    std::vector<cv::Mat> storedFeatures;
    cv::FileNode rootNode = fs.root();
    float minDistance = std::numeric_limits<float>::max(); // 记录最小距离

    for (cv::FileNodeIterator it = rootNode.begin(); it != rootNode.end(); ++it)
    {
        std::string featureName = (*it).name();
        if (featureName.find("feature_") == 0)
        {
            cv::Mat storedFeature;
            (*it) >> storedFeature;

            // 确保存储的特征也是float类型
            if (storedFeature.type() != CV_32FC1)
            {
                storedFeature.convertTo(storedFeature, CV_32FC1);
            }

            // 对存储的特征进行L2归一化
            normalize(storedFeature, storedFeature, 1.0, 0.0, cv::NORM_L2);

            qDebug() << "Stored feature" << featureName
                     << "size:" << storedFeature.size().width << " " << storedFeature.size().height
                     << "channels:" << storedFeature.channels()
                     << "type:" << storedFeature.type();

            if (normalizedInputFeature.size() != storedFeature.size() ||
                normalizedInputFeature.channels() != storedFeature.channels())
            {
                qDebug() << "Feature size mismatch!";
                continue;
            }

            // 计算余弦相似度（对于L2归一化的向量，可以直接用点积）
            float similarity = normalizedInputFeature.dot(storedFeature);
            float distance = 1.0 - similarity; // 转换为距离度量

            qDebug() << "Distance with" << featureName << ":" << distance;
            minDistance = std::min(minDistance, distance);
        }
    }

    fs.release();

    // 使用更合理的阈值（对于余弦距离来说，通常0.4-0.6是比较合理的范围）
    const float THRESHOLD = 0.5;
    qDebug() << "Minimum distance found:" << minDistance;

    return minDistance < THRESHOLD;
}

cv::Mat ClientHandler::QImageToCvMat(const QImage& Image)
{
    QImage image = Image;
    cv::Mat mat;
    switch (image.format())
    {
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied: // 添加对这个格式的支持
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_RGB32:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        break;
    default:
        qDebug() << "Converting unsupported format:" << image.format() << "to Format_RGBA8888";
        // 强制转换为支持的格式
        image = image.convertToFormat(QImage::Format_RGBA8888);
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        break;
    }
    // 4通道转3通道
    if (mat.channels() == 4)
    {
        cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
    }
    return mat;
}

cv::Mat ClientHandler::extractFeatureVector(const cv::Mat& faceImage)
{
    try
    {
        // 检查网络是否成功加载
        static cv::dnn::Net featureNet = cv::dnn::readNetFromONNX("D:\\Personal Data\\Qt\\face_test\\w600k_r50.onnx");
        if (featureNet.empty())
        {
            qDebug() << "Failed to load network model";
            return cv::Mat();
        }

        // 预处理输入图像
        cv::Mat processedImage;
        cv::Mat resizedImage;

        // 确保输入图像是BGR格式
        if (faceImage.channels() == 1)
        {
            cv::cvtColor(faceImage, processedImage, cv::COLOR_GRAY2BGR);
        }
        else
        {
            processedImage = faceImage.clone();
        }

        // 调整图像大小
        const int inputWidth = 112; // ResNet50 通常使用 112x112
        const int inputHeight = 112;
        cv::resize(processedImage, resizedImage, cv::Size(inputWidth, inputHeight));

        // 转换为浮点型并归一化
        cv::Mat float_img;
        resizedImage.convertTo(float_img, CV_32F);
        float_img = float_img / 255.0;

        // 标准化
        cv::Mat normalized;
        cv::subtract(float_img, cv::Scalar(0.5, 0.5, 0.5), normalized);
        cv::multiply(normalized, cv::Scalar(2.0, 2.0, 2.0), normalized);

        // 创建 blob
        cv::Mat blob = cv::dnn::blobFromImage(normalized,
                                              1.0, // scalefactor
                                              cv::Size(inputWidth, inputHeight),
                                              cv::Scalar(0, 0, 0), // mean
                                              true,                // swapRB
                                              false);              // crop

        // 打印 blob 的维度，用于调试
        qDebug() << "Blob dimensions:" << blob.dims << blob.size[0] << blob.size[1] << blob.size[2] << blob.size[3];

        // 设置网络输入
        featureNet.setInput(blob);

        // 获取网络输出层名称
        std::vector<std::string> outNames = featureNet.getUnconnectedOutLayersNames();
        if (outNames.empty())
        {
            qDebug() << "No output layers found";
            return cv::Mat();
        }

        // 获取特征向量
        std::vector<cv::Mat> outputs;
        featureNet.forward(outputs, outNames);

        if (outputs.empty())
        {
            qDebug() << "No output produced";
            return cv::Mat();
        }

        // 返回处理后的特征向量
        cv::Mat featureVector = outputs[0].clone();
        return featureVector.reshape(1, 1);
    }
    catch (const cv::Exception& e)
    {
        qDebug() << "OpenCV error:" << QString::fromStdString(e.what());
        return cv::Mat();
    }
    catch (const std::exception& e)
    {
        qDebug() << "Error in extractFeatureVector:" << e.what();
        return cv::Mat();
    }
}

void ClientHandler::forwordKickedOffline(const QJsonObject& json) // 把在线用户挤下线
{
    QMutexLocker locker(&dbMutex); // 锁住互斥量以确保线程安全
    QJsonObject qjsonObj;
    qjsonObj["tag"] = "home";
    qjsonObj["mode"] = "duplicate_logins";
    // 发送消息
    sendJsonResponse(qjsonObj);
}
