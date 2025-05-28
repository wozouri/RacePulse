#include "home.h"

#include <qbuffer.h>

#include "ui_home.h"

Home::Home(const QJsonObject& userInfo, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Home)
    , m_user_info(UserInfo::formJson(userInfo))
    , settings("settings.ini", QSettings::IniFormat)
    , m_apiclient(new ApiClient(m_user_info.usernum, this))
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    m_apiclient->connectToServer();

    setTime();
    setIcon();
    setAvatar();
    setFacebind();

    searchTerm(""); // 显示所有比赛

    ui->toolbu_close->installEventFilter(this);

    connect(m_apiclient, &ApiClient::dataReceived, this, &Home::recvHome);
    connect(this, &Home::sigRecvContestDetails, this, &Home::populateListViewFromJson); // 更新contest_details

    connect(qobject_cast<ListView*>(ui->listView_game_detail), &ListView::sigItemSelected, this, &Home::onListViewItemClicked);
    connect(this, &Home::sigFaceBindChanged, this, &Home::updateFaceBind);
}

Home::~Home()
{
    delete ui;
}

void Home::setTime()
{
    ui->lab_time->setShowMode(Label_time::Current_Time);
    ui->lab_date->setShowMode(Label_time::Current_Date);
}

void Home::setIcon()
{
    QPixmap pixmap_icon(":/images/logo.png");
    ui->lab_logo->setPixmap(pixmap_icon);
    ui->lab_logo->setScaledContents(true); // 贴合空间
}

void Home::setAvatar()
{
    ui->lab_avatar->setPixmap(m_user_info.m_avatar);
    ui->lab_avatar->setScaledContents(true);
}

void Home::setFacebind()
{
    QJsonObject qjson;
    qjson["tag"] = "home";
    qjson["mode"] = "face_bind";
    qjson["usernum"] = m_user_info.usernum;

    m_apiclient->sendJsonRequest(qjson);
}

void Home::updateFaceBind()
{
    if (facebind_flag == true)
    {
        ui->pu_facebind->setStyleSheet(
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
        ui->pu_facebind->setText("已绑定（点击进行修改）");
    }
    else if (facebind_flag == false)
    {
        ui->pu_facebind->setStyleSheet(
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
        ui->pu_facebind->setText("人脸未绑定（点击进行绑定）");
    }
}

void Home::searchTerm(const QString& TermName)
{
    if (!m_apiclient->connectToServer())
    {
        Dialog msgBox(this);
        msgBox.setNotice();
        msgBox.setText("请检查您的网络连接！");
        msgBox.exec();
        return;
    }

    QJsonObject jsonObj;
    jsonObj["tag"] = "home";
    jsonObj["mode"] = "search_term";
    jsonObj["search_term"] = TermName;
    m_apiclient->sendJsonRequest(jsonObj);
}

void Home::recvHome(const QJsonObject& recvJson)
{
    if (recvJson["tag"] != "home")
        return;
    if (recvJson["mode"] == "search_term")
    {
        emit sigRecvContestDetails(recvJson);
    }
    else if (recvJson["mode"] == "face_bind")
    {
        if (recvJson["status"] == "yes")
        {
            facebind_flag = true;
            emit sigFaceBindChanged();
        }
        else if (recvJson["status"] == "no")
        {
            facebind_flag = false;
            emit sigFaceBindChanged();
        }
    }
    else if (recvJson["mode"] == "shutdown")
    {
        Dialog msgBox(this);
        msgBox.setNotice();
        msgBox.setText("您的账号重复登陆，当前登录即将断开");
        msgBox.exec();

        m_apiclient->disconnect();
        this->close();
    }
}

void Home::mousePressEvent(QMouseEvent* event)
{
    QPoint pos = event->pos();
    int margin = 30;
    int margin_re = 8; // 鼠标dpi越高这个值得越高，不然跟不上鼠标移动
    ui->linee_search->clearFocus();
    if (event->button() == Qt::LeftButton)
    {
        dragPosition = event->globalPosition().toPoint() - this->geometry().topLeft();
        // 判断是否在窗口边缘
        if (pos.x() <= margin || pos.x() >= width() - margin ||
            pos.y() <= margin || pos.y() >= height() - margin)
        {
            moveFlag = 1;
            if (pos.x() <= margin_re || pos.x() >= width() - margin_re ||
                pos.y() <= margin_re || pos.y() >= height() - margin_re)
            {
                moveFlag = 0;
                resizeDirection = None; // 重置
                if (pos.x() <= margin_re)
                    resizeDirection |= Left;
                if (pos.x() >= width() - margin_re)
                    resizeDirection |= Right;
                if (pos.y() <= margin_re)
                    resizeDirection |= Top;
                if (pos.y() >= height() - margin_re)
                    resizeDirection |= Bottom;

                // 根据调整方向设置光标形状
                updateCursorShape();
            }
        }
        event->accept();
    }
}

void Home::closeEvent(QCloseEvent* event)
{
    // 保存头像数据
    settings.beginGroup(m_user_info.usernum);
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    m_user_info.m_avatar.save(&buffer, "PNG");
    QString base64Image = byteArray.toBase64();
    settings.setValue("avatar_data", base64Image);
    settings.endGroup();

    // 执行默认关闭操作
    event->accept();
}

void Home::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿

    // 设置绘制区域为整个窗口
    painter.drawRect(rect());

    // 根据moveFlag设置不同的背景颜色
    if (moveFlag == 1)
    {
        // 黄色背景
        painter.fillRect(rect(), QColor(255, 255, 0, 100)); // 半透明黄色
    }
    else if (moveFlag == 0)
    {
        // 绿色背景
        painter.fillRect(rect(), QColor(0, 255, 0, 100)); // 半透明绿色
    }

    // 调用父类的paintEvent，确保其他绘制正常进行
    QWidget::paintEvent(event);
}

void Home::mouseMoveEvent(QMouseEvent* event)
{
    QPoint pos = event->pos();

    // // 在鼠标移动时动态更新光标形状
    // resizeDirection = None;
    // if (pos.x() <= margin_re)
    //     resizeDirection |= Left;
    // if (pos.x() >= width() - margin_re)
    //     resizeDirection |= Right;
    // if (pos.y() <= margin_re)
    //     resizeDirection |= Top;
    // if (pos.y() >= height() - margin_re)
    //     resizeDirection |= Bottom;

    // updateCursorShape();

    if (event->buttons() & Qt::LeftButton)
    {
        if (moveFlag == 1) // 移动窗口
        {
            this->move(event->globalPosition().toPoint() - dragPosition);
            update();
        }
        else if (moveFlag == 0 && resizeDirection != None) // 调整窗口大小
        {
            QPoint currentPos = event->globalPosition().toPoint();
            QRect geometry = this->geometry();

            if (resizeDirection & Left)
            {
                int newWidth = geometry.width() + (geometry.left() - currentPos.x());
                geometry.setLeft(currentPos.x());

                if (newWidth >= minimumWidth())
                {
                    this->setGeometry(geometry);
                }
            }

            if (resizeDirection & Right)
            {
                geometry.setWidth(currentPos.x() - geometry.left());
                if (geometry.width() >= minimumWidth())
                {
                    this->setGeometry(geometry);
                }
            }

            if (resizeDirection & Top)
            {
                int newHeight = geometry.height() + (geometry.top() - currentPos.y());
                geometry.setTop(currentPos.y());
                if (newHeight >= minimumHeight())
                {
                    this->setGeometry(geometry);
                }
            }

            if (resizeDirection & Bottom)
            {
                geometry.setHeight(currentPos.y() - geometry.top());
                if (geometry.height() >= minimumHeight())
                {
                    this->setGeometry(geometry);
                }
            }
        }
        event->accept();
    }
}

void Home::updateCursorShape()
{
    switch (resizeDirection)
    {
    case Left:
    case Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case Top:
    case Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case Left | Top:
    case Right | Bottom:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case Left | Bottom:
    case Right | Top:
        setCursor(Qt::SizeBDiagCursor);
        break;
    default:
        unsetCursor(); // 恢复默认光标
        break;
    }
}

void Home::mouseReleaseEvent(QMouseEvent* event)
{
    moveFlag = -1;
    resizeDirection = None;
    unsetCursor(); // 释放鼠标时恢复默认光标
    update();
}

void Home::setGeometry(const QRect& rect)
{
    // 添加合理性检查
    if (rect.width() < minimumWidth() || rect.height() < minimumHeight())
    {
        // qDebug() << "Attempted to set invalid geometry:" << rect;
        return;
    }

    // // 记录几何变化日志
    // qDebug() << "Geometry changed:"
    //          << "X:" << rect.x()
    //          << "Y:" << rect.y()
    //          << "Width:" << rect.width()
    //          << "Height:" << rect.height();

    update(); // 窗口缩放时变为绿色
    QWidget::setGeometry(rect);
}

bool Home::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->toolbu_close)
    {
        switch (event->type())
        {
        case QEvent::Enter:
            ui->toolbu_close->setIcon(QIcon(":/images/close_w.png"));
            ui->toolbu_close->setIconSize(QSize(14, 14));
            ui->toolbu_close->setStyleSheet(
                "QToolButton {"
                "border: none;"
                "background: red;"
                "}"

            );
            break;
        case QEvent::Leave:
            ui->toolbu_close->setIcon(QIcon(":/images/close_b.png"));
            ui->toolbu_close->setStyleSheet(
                "QToolButton{"
                "border: none;"
                "background: transparent;"
                "border-radius: 3px;"
                "}");
            break;
        default:
            break;
        }
    }
    return QObject::eventFilter(obj, event);
}

void Home::on_toolbu_close_clicked()
{
    this->close();
}

void Home::on_toolbu_max_clicked()
{
    if (tool_max_flag == 0) // max ->　normal
    {
        ui->toolbu_max->setStyleSheet(
            "QToolButton{"
            "border:none;"
            "qproperty-icon:url(:/images/maxmize.svg);"
            "qproperty-iconSize:14px 17px;"
            "}"
            "QToolButton:hover{"
            "border:none;"
            "background: rgb(234,235,237);"
            "}");
        this->showNormal();
        tool_max_flag = 1;
    }
    else if (tool_max_flag == 1)
    {
        ui->toolbu_max->setStyleSheet(
            "QToolButton{"
            "border:none;"
            "qproperty-icon:url(:/images/normal.svg);"
            "qproperty-iconSize:14px 17px;"
            "}"
            "QToolButton:hover{"
            "border:none;"
            "background: rgb(234,235,237);"
            "}");
        this->showMaximized();
        tool_max_flag = 0;
    }
}

void Home::on_toolbu_min_clicked()
{
    this->showMinimized();
}

void Home::on_toolbu_search_clicked()
{
    QString search_text = ui->linee_search->text();
    if (search_text == "" || search_text == LineEdit::SEARCH_PLH)
        return;
    searchTerm(search_text);
}

void Home::onListViewItemClicked(int index)
{
    if (index < 0 || index >= current_contest.size())
    {
        qDebug() << "Index out of range:" << index;
        return;
    }

    QJsonValue value = current_contest.at(index);
    if (!value.isObject())
    {
        qDebug() << "Selected item is not a valid QJsonObject";
        return;
    }

    if (m_contest.isNull())
    {
        m_contest = new Contest(current_contest.at(index).toObject(), m_apiclient, nullptr);
    }

    m_contest->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动释放内存
    m_contest->setWindowFlags(Qt::Window);         // 设置为独立窗口
    m_contest->show();
    m_contest->activateWindow();
}

void Home::populateListViewFromJson(const QJsonObject& jsonObj)
{
    // 获取赛事结果列表
    QJsonArray results = jsonObj["results"].toArray();

    current_contest = results;

    // 创建 QStandardItemModel
    QStandardItemModel* model = new QStandardItemModel(this);

    // 遍历赛事数据并将其添加到 model 中
    for (const QJsonValue& value : results)
    {
        QJsonObject contest = value.toObject();

        // 获取赛事信息
        QString contestName = contest.value("contest_name").toString("Unknown Contest");
        QString startTime = contest.value("start_time").toString("TBD");
        QString location = contest.value("location").toString("Unknown Location");
        QString contestLogo = contest["contest_logo"].toString(); // 赛事图标路径

        // 创建新的 QStandardItem 并设置数据
        QStandardItem* item = new QStandardItem();

        QPixmap pixmap;
        if (!pixmap.load(contestLogo))
        {
            pixmap.load(":/images/logo.png"); // 使用默认图标
        }
        item->setData(pixmap, Qt::DecorationRole);   // 设置图标
        item->setData(contestName, Qt::DisplayRole); // 设置赛事名称
        item->setData(startTime, Qt::UserRole);      // 设置开始时间

        // 添加该项到模型
        model->appendRow(item);
    }

    // 设置 QListView 的模型
    ui->listView_game_detail->setModel(model);

    // 显示列表视图
    ui->listView_game_detail->show();
}

void Home::on_pu_facebind_clicked()
{
    // 绑定或者修改人脸信息
    if (m_facedetection.isNull())
    {
        m_facedetection = new FaceDetection(m_apiclient, facebind_flag ? "modify" : "save", this);
        m_facedetection->setWindowFlags(Qt::Window);
    }
    m_facedetection->setMode(facebind_flag ? "modify" : "save");
    connect(m_facedetection, &FaceDetection::sigFaceSaveSuccess, this, &Home::on_face_save_success);

    m_facedetection->setWindowModality(Qt::ApplicationModal);
    m_facedetection->show();
    m_facedetection->activateWindow();
}

bool Home::get_facebind_flag() const
{
    return facebind_flag;
}

void Home::on_face_save_success()
{
    facebind_flag = true;
    updateFaceBind();
}
