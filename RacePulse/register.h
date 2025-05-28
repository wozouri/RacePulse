#ifndef REGISTER_H
#define REGISTER_H

#include <network_modules/apiclient.h>
#include <qregularexpression.h>

#include <QRegularExpressionValidator>
#include <QWidget>

#include "rewritten_widgets/lineedit.h"

namespace Ui
{
class Register;
}

class Register : public QWidget
{
    Q_OBJECT

public:
    explicit Register(ApiClient* apiclient, QWidget* parent = nullptr);
    ~Register();
    void setVal();
    void setAva();

    void paintEvent(QPaintEvent* event);

    void mousePressEvent(QMouseEvent* event);

    void sendRegister();
    void recvRegister(const QJsonObject& recvJson);

    void receAvator(const QPixmap& pixmap);

    QString encryptPassword(const QString& password);

private slots:
    void on_linee_answer_textChanged(); // 文本改变 判断能不能提交

    void on_linee_username_textChanged(const QString& arg1);

    void on_linee_password_textChanged(const QString& arg1);

    void on_linee_password_again_textChanged(const QString& arg1);

    void on_radioButton_clicked();

    void on_pu_register_clicked();

    void on_register_reset();

signals:
    void sigRegisterSucceed(const QJsonObject& qjson, const QPixmap avatar);
    void sigRegisterFailed();

private:
    Ui::Register* ui;
    ApiClient* m_apiclient;
    QPixmap m_avatar;

    QRegularExpressionValidator* validator;
};

#endif // REGISTER_H
