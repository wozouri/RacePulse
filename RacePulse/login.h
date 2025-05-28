#ifndef LOGIN_H
#define LOGIN_H

#include <Register.h>
#include <qsettings.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QMutexLocker>
#include <QNetworkProxy>
#include <QPainter>
#include <QPointer>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTcpSocket>
#include <QTimer>
#include <QWidget>

#include "network_modules/apiclient.h"
#include "rewritten_widgets/lineedit.h"

namespace Ui
{
class Login;
}

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget* parent = nullptr);
    ~Login();
    void setIcon();
    void setVal();
    void setAva(const QPixmap& pixmap);
    void setTimer();
    void setSettings();

    void saveSettings();

    void paintEvent(QPaintEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);

    void sendLogin();
    void recvLogin(const QJsonObject& recvJson);

    void cleanupConnections();

    QString encryptPassword(const QString& password);
    QString xorEncryptDecrypt(const QString& data, const QString& key = "xorEncryptDecrypt_key");

signals:
    void sigloginSucceed(const QJsonObject& qjson);
    void sigloginFailed();

private slots:
    void on_linee_answer_textChanged();
    void on_linee_usernum_textChanged(const QString& arg1);
    void on_linee_password_textChanged(const QString& arg1);

    void on_toolButton_clicked();
    void updateGradient();

    void on_pu_register_clicked(); // 注册界面跳转
    void on_pu_login_clicked();

    void on_toolButton_close_clicked();

    void on_login_reset();                                                        // 登录失败重置状态
    void on_register_message_fill(const QJsonObject qjson, const QPixmap avatar); // 注册成功后账号的填充和头像的显示

private:
    Ui::Login* ui;
    ApiClient* m_apiclient;

    QSettings settings;
    /*存储
    /  avatar            QString(Base64)
    /  usernum           QString
    /  password          QString
    /  remeberPassword   bool
    */
    QString settings_usernum;
    QPixmap settings_avatar;

    QPixmap default_avatar = QPixmap(":/images/avatar.png");

    int offset = 0;   // 渐变尺度;
    int moveFlag = 0; // 用于判断是否要拖动窗口
    QPoint dragPosition;

    QRegularExpressionValidator* validator; // 验证器
    QPointer<Register> regis;               // 注册账号窗口对象
};

#endif // LOGIN_H
