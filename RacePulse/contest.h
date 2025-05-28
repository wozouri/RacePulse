#ifndef CONTEST_H
#define CONTEST_H

#include <custom_controls/facedetection.h>
#include <qpointer.h>

#include <QDateTime>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QWidget>

#include "network_modules/apiclient.h"
#include "qjsonobject.h"
#include "qtimer.h"

namespace Ui
{
class Contest;
}

struct ContestInfo
{
    QString contest_id;
    QString contest_name;
    QString contest_logo;
    QDateTime start_time; // 使用 QDateTime
    QDateTime end_time;
    QString description;
    QString status;
    QString creator_nickname; // 服务端 id -> creator_nickname
    QString contest_password;

    // 从 QJsonObject 创建 ContestInfo
    static ContestInfo fromJson(const QJsonObject& obj)
    {
        ContestInfo info;
        info.contest_id = obj["contest_id"].toString();
        info.contest_name = obj["contest_name"].toString();
        info.contest_logo = obj["contest_logo"].toString();
        info.start_time = QDateTime::fromString(obj["start_time"].toString(), Qt::ISODate);
        info.end_time = QDateTime::fromString(obj["end_time"].toString(), Qt::ISODate);
        info.description = obj["description"].toString();
        info.status = obj["status"].toString();
        info.creator_nickname = obj["creator_nickname"].toString();
        info.contest_password = obj["contest_password"].toString();
        return info;
    }
};
class FaceDetection;
class Contest : public QWidget
{
    Q_OBJECT

public:
    explicit Contest(const QJsonObject contest_info, ApiClient* apiclient, QWidget* parent = nullptr);
    ~Contest();

    void setContestInfo(const ContestInfo& info);
    ContestInfo getContestInfo() const;
    void page_ctpwd_init();
    void page_ctmain_init();

    void setVal();
private slots:
    void on_face_check_success();

    void on_pu_ctpwd_clicked();

    void on_linee_ctpwd_textChanged(const QString& arg1);

    void on_pu_sign_clicked();

private:
    Ui::Contest* ui;

    ApiClient* m_apiclient;
    ContestInfo m_contest_info; // 使用结构体作为成员变量
    QPointer<FaceDetection> m_facedetection;

    QRegularExpressionValidator* validator; // 验证器
};

#endif // CONTEST_H
