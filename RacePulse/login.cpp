#include "login.h"

#include <qcryptographichash.h>

#include "custom_controls/dialog.h"
#include "qdialog.h"
#include "ui_login.h"

Login::Login(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Login)
    , settings("settings.ini", QSettings::IniFormat)
{
    ui->setupUi(this);
    // 客户端与服务端的信息收发
    m_apiclient = new ApiClient("", this);
    m_apiclient->connectToServer();

    this->setFocus();
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    // 初始化icon
    setIcon();
    setVal();
    setAva(default_avatar);
    setTimer();
    setSettings();

    connect(m_apiclient, &ApiClient::dataReceived, this, &Login::recvLogin);
    connect(this, &Login::sigloginFailed, this, &Login::on_login_reset);
}

Login::~Login()
{
    delete ui;
}

void Login::setIcon()
{
    QPixmap pixmap_icon(":/images/logo.png");
    ui->lab_logo->setPixmap(pixmap_icon);
    ui->lab_logo->setScaledContents(true); // 贴合空间
}

void Login::setVal()
{
    QRegularExpression regExp("^[a-zA-Z0-9]{1,12}$"); // 设置验证器
    Login::validator = new QRegularExpressionValidator(regExp, this);
    ui->linee_usernum->setValidator(validator);
    ui->linee_password->setValidator(validator);
}

void Login::setAva(const QPixmap& pixmap)
{
    if (pixmap.isNull())
    {
        ui->lab_avatar->setPixmap(default_avatar);
        ui->lab_avatar->setScaledContents(true);
    }
    else
    {
        ui->lab_avatar->setPixmap(pixmap);
        ui->lab_avatar->setScaledContents(true);
    }
}

void Login::setTimer()
{
    QTimer* timer = new QTimer(this); // 建立计时器 来更新动态背景
    connect(timer, &QTimer::timeout, this, &Login::updateGradient);
    timer->start(50);
}

void Login::setSettings()
{
    // 获取最近一次的登录账号
    settings_usernum = settings.value("lastUsernum", "").toString(); // 从设置中读取最后一次登录的用户账号
    if (!settings_usernum.isEmpty())
    {
        ui->linee_usernum->setText(settings_usernum);

        // 根据 settings_usernum 获取其他信息
        settings.beginGroup(settings_usernum); // 使用 settings_usernum 作为组名

        bool rememberPassword = settings.value("rememberPassword", false).toBool();
        QString password = xorEncryptDecrypt(settings.value("password", "").toString());
        QByteArray base64Data = settings.value("avatar_data", "").toString().toUtf8();
        QByteArray imageData = QByteArray::fromBase64(base64Data);

        if (!settings_avatar.loadFromData(imageData, "PNG"))
        {
            qDebug() << "Error: failed to load image from data";
        }
        setAva(settings_avatar);

        if (rememberPassword)
        {
            ui->ch_rpwd->setCheckState(Qt::Checked);
            if (!password.isEmpty())
            {
                ui->linee_password->setText(password);
                ui->linee_password->setEchoMode(QLineEdit::Password);
            }
        }

        settings.endGroup(); // 结束读取当前用户对应的组
    }
}

void Login::saveSettings()
{
    QString usernum = ui->linee_usernum->text();
    settings.setValue("lastUsernum", usernum); // 保存最近登录的用户账号

    settings.beginGroup(usernum); // 使用当前登录账号作为组名
    settings.setValue("usernum", usernum);
    if (ui->ch_rpwd->isChecked())
    {
        settings.setValue("rememberPassword", true);
        settings.setValue("password", xorEncryptDecrypt(ui->linee_password->text()));
    }
    else
    {
        settings.setValue("rememberPassword", false);
        settings.setValue("password", "");
    }
    settings.endGroup();
}

void Login::on_linee_answer_textChanged()
{
    if (!ui->linee_usernum->text().isEmpty() && ui->linee_usernum->text() != LineEdit::UNUM_PLH &&
        !ui->linee_password->text().isEmpty() && ui->linee_password->text() != LineEdit::PWD_PLH)
    {
        ui->pu_login->setEnabled(true);
        ui->pu_login->setStyleSheet(
            "QPushButton {"
            "font: 12pt 'Microsoft YaHei UI';"
            "color: white;"
            "border-radius: 15px;"
            "background:"
            "linear-gradient(to bottom right, rgba(255,255,255,0.2), transparent 50%),"
            "qlineargradient(x1: 1, y1: 0, x2: 1, y2: 1, stop: 0 #a0a0ff, stop: 1 #6060c0);"
            "}"
            "QPushButton:hover {"
            "background:"
            "linear-gradient(to bottom right, rgba(255,255,255,0.3), transparent 50%),"
            "qlineargradient(x1: 1, y1: 0, x2: 1, y2: 1, stop: 0 #b0b0ff, stop: 1 #8080d0);"
            "}"
            "QPushButton:pressed {"
            "background:"
            "linear-gradient(to bottom right, rgba(0,0,0,0.1), transparent 50%),"
            "qlineargradient(x1: 1, y1: 0, x2: 1, y2: 1, stop: 0 #4040c0, stop: 1 #3030a0);"
            "padding: -2px -2px 2px 2px;"
            "}");
    }
    else
    {
        ui->pu_login->setEnabled(false);
        ui->pu_login->setStyleSheet(
            "QPushButton {"
            "font: 12pt 'Microsoft YaHei UI';"
            "color: white;"
            "border-radius: 15px;"
            "background:"
            "linear-gradient(to bottom right, rgba(255,255,255,0.2), transparent 50%),"
            "qlineargradient(x1: 1, y1: 0, x2: 1, y2: 1, stop: 0 #c0d0ff, stop: 1 #a0b0e0);"
            "}");
    }
}

void Login::on_linee_usernum_textChanged(const QString& arg1)
{
    if (ui->linee_usernum->text() == settings_usernum)
    {
        setAva(settings_avatar);
    }
    else
    {
        setAva(default_avatar);
    }
    on_linee_answer_textChanged();
}

void Login::on_linee_password_textChanged(const QString& arg1)
{
    on_linee_answer_textChanged();
}
void Login::updateGradient() // 更新渐变背景
{
    offset = (offset + 1) % INT_MAX;
    update();
}

void Login::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient gradient(0, 0, width(), height());

    int r1 = 255;
    int g1 = 220 + (sin(offset * 0.04) * 25); // 减小振幅和频率
    int r2 = 172;
    int g2 = 214; // 稍微调低绿色分量
    int b2 = 245; // 稍微调低蓝色分量

    gradient.setColorAt(0.0, QColor(r1, g1, 235));
    gradient.setColorAt(1.0, QColor(r2, g2, b2));

    painter.fillRect(rect(), gradient); // 使用fillRect代替drawRect

    // 添加阴影的绘制
    painter.setPen(QPen(QColor(0, 0, 0, 127), 1));   // 设置阴影的画笔
    painter.setOpacity(0.1);                         // 设置阴影的透明度
    painter.drawRect(rect().adjusted(1, 1, -1, -1)); // 绘制阴影矩形
}

void Login::mousePressEvent(QMouseEvent* event) // 点击窗口空白让输入框失去焦点 并且判断拖拽窗口
{
    QPoint pos = event->pos();
    int margin = 30;
    ui->linee_usernum->clearFocus();
    ui->linee_password->clearFocus();

    if (event->button() == Qt::LeftButton)
    {
        dragPosition = event->globalPosition().toPoint() - this->geometry().topLeft();
        if (pos.x() <= margin || pos.x() >= width() - margin ||
            pos.y() <= margin || pos.y() >= height() - margin)
        { // 如果点击在边缘
            moveFlag = 1;
        }
        event->accept();
    }
}

void Login::mouseMoveEvent(QMouseEvent* event) // 拖拽移动窗口位置
{
    if (event->buttons() & Qt::LeftButton && this->rect().contains(event->pos()) && moveFlag == 1)
    {
        this->move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

void Login::mouseReleaseEvent(QMouseEvent* event) // 重置移动状态
{
    moveFlag = 0;
}

void Login::on_toolButton_clicked()
{
    this->close();
}

void Login::on_pu_register_clicked()
{
    if (regis.isNull())
    {
        regis = new Register(m_apiclient, this);
    }
    connect(regis, &Register::sigRegisterSucceed,
            this, &Login::on_register_message_fill,
            Qt::QueuedConnection); // 考虑使用队列连接
    regis->setWindowModality(Qt::ApplicationModal);
    regis->show();
    regis->activateWindow();
}

void Login::sendLogin()
{
    if (!m_apiclient->connectToServer())
    {
        Dialog msgBox(this);
        msgBox.setNotice();
        msgBox.setText("请检查您的网络连接！");
        msgBox.exec(); // 模态对话框，阻塞程序运行，至对话框关闭
        return;
    }
    QJsonObject jsonObj;
    jsonObj["tag"] = "login";
    jsonObj["usernum"] = ui->linee_usernum->text();
    jsonObj["password"] = encryptPassword(ui->linee_password->text());

    m_apiclient->sendJsonRequest(jsonObj);
}

void Login::recvLogin(const QJsonObject& recvJson)
{
    if (recvJson["tag"] != "login")
        return;
    if (recvJson["result"] == "success")
    {
        saveSettings();
        // 登录成功 发送信号给主函数
        emit sigloginSucceed(recvJson);
        m_apiclient->disconnect();
        this->close();
    }
    else if (recvJson["result"] == "fail")
    {
        Dialog msgBox(this);
        msgBox.setNotice();
        msgBox.setText(recvJson["reason"].toString());
        msgBox.exec(); // 模态对话框，阻塞程序运行，至对话框关闭
        emit sigloginFailed();
    }
}

QString Login::encryptPassword(const QString& password)
{
    QByteArray byteArray = password.toUtf8();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(byteArray);
    return hash.result().toHex(); // 返回加密后的密码（以十六进制字符串形式）
}

QString Login::xorEncryptDecrypt(const QString& data, const QString& key)
{
    QByteArray result = data.toUtf8();
    int keyLength = key.length();
    for (int i = 0; i < result.size(); ++i)
    {
        result[i] = result[i] ^ key[i % keyLength].toLatin1();
    }
    return QString(result); // 返回解密或加密后的字符串
}

void Login::on_pu_login_clicked()
{
    // 锁定登录按钮及输入框 并 发送登录信息等待验证
    ui->pu_login->setEnabled(false);
    ui->linee_usernum->setEnabled(false);
    ui->linee_password->setEnabled(false);

    ui->pu_login->setText("登录中...");
    ui->pu_login->setStyleSheet(
        "QPushButton {"
        "font: 12pt 'Microsoft YaHei UI';"
        "color: white;"
        "border-radius: 15px;"
        "background:"
        "linear-gradient(to bottom right, rgba(255,255,255,0.2), transparent 50%),"
        "qlineargradient(x1: 1, y1: 0, x2: 1, y2: 1, stop: 0 #c0d0ff, stop: 1 #a0b0e0);"
        "}");
    sendLogin();
}

void Login::on_toolButton_close_clicked()
{
    this->close();
}

void Login::on_login_reset()
{
    ui->pu_login->setEnabled(true);
    ui->linee_usernum->setEnabled(true);
    ui->linee_password->setEnabled(true);
    ui->pu_login->setText("登录");
    ui->pu_login->setStyleSheet(
        "QPushButton {"
        "font: 12pt 'Microsoft YaHei UI';"
        "color: white;"
        "border-radius: 15px;"
        "background:"
        "linear-gradient(to bottom right, rgba(255,255,255,0.2), transparent 50%),"
        "qlineargradient(x1: 1, y1: 0, x2: 1, y2: 1, stop: 0 #a0a0ff, stop: 1 #6060c0);"
        "}"
        "QPushButton:hover {"
        "background:"
        "linear-gradient(to bottom right, rgba(255,255,255,0.3), transparent 50%),"
        "qlineargradient(x1: 1, y1: 0, x2: 1, y2: 1, stop: 0 #b0b0ff, stop: 1 #8080d0);"
        "}"
        "QPushButton:pressed {"
        "background:"
        "linear-gradient(to bottom right, rgba(0,0,0,0.1), transparent 50%),"
        "qlineargradient(x1: 1, y1: 0, x2: 1, y2: 1, stop: 0 #4040c0, stop: 1 #3030a0);"
        "padding: -2px -2px 2px 2px;"
        "}");
}

void Login::on_register_message_fill(const QJsonObject qjson, const QPixmap avatar)
{
    qDebug() << "注册信息传输成功";
    ui->lab_avatar->setPixmap(avatar);
    if (qjson.contains("usernum"))
    {
        ui->linee_usernum->setText(qjson["usernum"].toString());
    }
}
