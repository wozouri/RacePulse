#include "register.h"

#include <custom_controls/dialog.h>
#include <qbuffer.h>
#include <qcryptographichash.h>
#include <qpainter.h>

#include "ui_register.h"

Register::Register(ApiClient* apiclient, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Register)
    , m_apiclient(apiclient)
{
    ui->setupUi(this);
    this->setFocus();
    // 初始话窗口，只有关闭
    setWindowFlags(Qt::Dialog);

    setVal();
    setAva();

    connect(m_apiclient, &ApiClient::dataReceived, this, &Register::recvRegister);
    connect(this, &Register::sigRegisterFailed, this, &Register::on_register_reset);
}

Register::~Register()
{
    delete ui;
}

void Register::setVal()
{
    // 宽松
    QRegularExpression passwordRegExp("^[a-zA-Z0-9]{1,12}$");
    ui->linee_password->setValidator(new QRegularExpressionValidator(passwordRegExp, ui->linee_password));
    ui->linee_password_again->setValidator(new QRegularExpressionValidator(passwordRegExp, ui->linee_password_again));

    QRegularExpression usernameRegExp("^[a-zA-Z0-9_]{1,12}$");
    ui->linee_nickname->setValidator(new QRegularExpressionValidator(usernameRegExp, ui->linee_nickname));

    // 严格
    // QRegularExpression passwordRegExp("^(?=.*[a-z])(?=.*[A-Z])(?=.*\\d)[a-zA-Z\\d]{8,16}$");
    // ui->linee_password->setValidator(new QRegularExpressionValidator(passwordRegExp, ui->linee_password));
    // ui->linee_password_again->setValidator(new QRegularExpressionValidator(passwordRegExp, ui->linee_password_again));

    // QRegularExpression usernameRegExp("^[a-zA-Z0-9_]{4,12}$");
    // ui->linee_username->setValidator(new QRegularExpressionValidator(usernameRegExp, ui->linee_username));

    // 使用父对象自动管理内存
}

void Register::setAva()
{
    m_avatar = QPixmap(":/images/avatar.png");

    ui->lab_avatar->setPixmap(m_avatar); // 默认头像
    ui->lab_avatar->setScaledContents(true);
}

void Register::paintEvent(QPaintEvent* event) // 初始化背景
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 反锯齿
    QPixmap pixmap(":/images/bg_p-b.png");
    painter.drawPixmap(0, 0, width(), height(), pixmap);
    QIcon icon(":/images/logo.png");
    setWindowIcon(icon);
    // 获取标题栏
    setWindowTitle("赛搏");
}

void Register::mousePressEvent(QMouseEvent* event) // 点击窗口空白让输入框失去焦点 并且判断拖拽窗口
{
    ui->linee_nickname->clearFocus();
    ui->linee_password->clearFocus();
    ui->linee_password_again->clearFocus();
}

void Register::sendRegister()
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    m_avatar.save(&buffer, "PNG");
    QString base64Image = byteArray.toBase64();

    QJsonObject jsonObj;
    jsonObj["tag"] = "register";
    jsonObj["nickname"] = ui->linee_nickname->text();
    jsonObj["password"] = encryptPassword(ui->linee_password->text());
    jsonObj["avatar_data"] = base64Image;
    m_apiclient->sendJsonRequest(jsonObj);
}

void Register::recvRegister(const QJsonObject& recvJson)
{
    if (recvJson["tag"] != "register")
        return;
    if (recvJson["result"] == "success")
    {
        Dialog msgBox(this);
        msgBox.setNotice();
        msgBox.setText("注册成功！\n 您的账号为：" + recvJson["usernum"].toString());
        msgBox.exec(); // 模态对话框，阻塞程序运行，至对话框关闭
        emit sigRegisterSucceed(recvJson, m_avatar);
        this->close();
    }
    else if (recvJson["result"] == "fail")
    {
        Dialog msgBox(this);
        msgBox.setNotice();
        msgBox.setText(recvJson["reason"].toString());
        msgBox.exec(); // 模态对话框，阻塞程序运行，至对话框关闭
        emit sigRegisterFailed();
    }
}

QString Register::encryptPassword(const QString& password)
{
    QByteArray byteArray = password.toUtf8();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(byteArray);
    return hash.result().toHex(); // 返回加密后的密码（以十六进制字符串形式）
}

void Register::on_linee_answer_textChanged()
{
    if (!ui->linee_nickname->text().isEmpty() && ui->linee_nickname->text() != LineEdit::NICK_PLH &&
        !ui->linee_password->text().isEmpty() && ui->linee_password->text() != LineEdit::PWD_PLH &&
        !ui->linee_password_again->text().isEmpty() && ui->linee_password_again->text() != LineEdit::PWD_AGAIN_PLH &&
        ui->radioButton->isChecked())
    {
        ui->pu_register->setEnabled(true);
        ui->pu_register->setStyleSheet(
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
        ui->pu_register->setEnabled(false);
        ui->pu_register->setStyleSheet(
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

void Register::on_linee_username_textChanged(const QString& arg1)
{
    on_linee_answer_textChanged();
}

void Register::on_linee_password_textChanged(const QString& arg1)
{
    on_linee_answer_textChanged();
}

void Register::on_linee_password_again_textChanged(const QString& arg1)
{
    on_linee_answer_textChanged();
}

void Register::on_radioButton_clicked()
{
    on_linee_answer_textChanged();
}

void Register::receAvator(const QPixmap& pixmap) // 收到用户选择的头像然后做相应处理
{
    // 处理圆角并更新图片
    m_avatar = pixmap;
    ui->lab_avatar->setPixmap(pixmap);
    ui->lab_avatar->setScaledContents(true);
}

void Register::on_pu_register_clicked()
{
    if (!m_apiclient->connectToServer())
    {
        Dialog msgBox(this);
        msgBox.setNotice();
        msgBox.setText("请检查您的网络连接！");
        msgBox.exec();
        return;
    }
    ui->linee_nickname->setEnabled(false);
    ui->linee_password->setEnabled(false);
    ui->linee_password_again->setEnabled(false);
    ui->radioButton->setEnabled(false);
    ui->pu_register->setEnabled(false);
    ui->pu_register->setStyleSheet(
        "QPushButton {"
        "font: 12pt 'Microsoft YaHei UI';"
        "color: white;"
        "border-radius: 15px;"
        "background:"
        "linear-gradient(to bottom right, rgba(255,255,255,0.2), transparent 50%),"
        "qlineargradient(x1: 1, y1: 0, x2: 1, y2: 1, stop: 0 #c0d0ff, stop: 1 #a0b0e0);"
        "}");
    sendRegister();
}

void Register::on_register_reset()
{
    ui->linee_nickname->setEnabled(true);
    ui->linee_password->setEnabled(true);
    ui->linee_password_again->setEnabled(true);
    ui->radioButton->setEnabled(true);
    ui->pu_register->setEnabled(true);
    ui->pu_register->setStyleSheet(
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
