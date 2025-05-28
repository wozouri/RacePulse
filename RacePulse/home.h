#ifndef HOME_H
#define HOME_H

#include <Contest.h>
#include <custom_controls/dialog.h>
#include <network_modules/apiclient.h>
#include <qboxlayout.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <QGroupBox>
#include <QLCDNumber>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTableWidget>
#include <QWidget>

#include "qevent.h"
#include "qjsonarray.h"
#include "qpainter.h"
#include "qsettings.h"
#include "rewritten_widgets/label.h"

namespace Ui
{
class Home;
}

struct UserInfo
{
    QString usernum;
    QString nickname;
    QString role;
    QPixmap m_avatar;

    static UserInfo formJson(const QJsonObject& obj)
    {
        UserInfo info;
        info.usernum = obj["usernum"].toString();
        info.nickname = obj["nickname"].toString();
        info.role = obj["role"].toString();

        QByteArray base64Data = obj["avatar_data"].toString().toUtf8();
        QByteArray imageData = QByteArray::fromBase64(base64Data);

        if (!info.m_avatar.loadFromData(imageData, "PNG"))
        {
            qDebug() << "Error: failed to load image from data";
        }

        return info;
    }
};

class Home : public QWidget
{
    Q_OBJECT

    enum ResizeDirection
    {
        None = 0,
        Left = 1,
        Right = 2,
        Top = 4,
        Bottom = 8
    };

public:
    explicit Home(const QJsonObject& userInfo, QWidget* parent = nullptr);
    ~Home();

    void setTime();
    void setIcon();
    void setAvatar();
    void setFacebind();

    void updateFaceBind();

    void searchTerm(const QString& TermName);

    void recvHome(const QJsonObject& recvJson);

    void updateContestDetails();

    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);

    void paintEvent(QPaintEvent* event);           // move/change window
    bool eventFilter(QObject* obj, QEvent* event); // toolbu_close
    void updateCursorShape();                      // move/change window
    void setGeometry(const QRect& rect);           // debug

    void populateListViewFromJson(const QJsonObject& jsonObj);

    bool get_facebind_flag() const; // 查询当前的用户人脸绑定状态

    void closeEvent(QCloseEvent* event); // 保存头像数据到本地配置文件
signals:
    void sigRecvContestDetails(const QJsonObject& jsonObj);

    void sigFaceBindChanged();

private slots:
    void on_face_save_success();

    void on_toolbu_close_clicked();
    void on_toolbu_max_clicked();
    void on_toolbu_min_clicked();
    void on_toolbu_search_clicked();

    void onListViewItemClicked(int index); // 点击比赛详细项，跳转比赛页面

    void on_pu_facebind_clicked();

private:
    Ui::Home* ui;

    UserInfo m_user_info; // 使用结构体作为成员变量

    QSettings settings;
    /*存储
    /  avatar            QString(Base64)
    /  usernum           QString
    /  password          QString
    /  remeberPassword   bool
    */

    int moveFlag = -1; //-1:normal  0: change  1:move
    int resizeDirection = None;
    QPoint dragPosition;

    bool tool_max_flag = 1; // 0:max 1:normal
    bool facebind_flag = 0; // status:  0:no 1:yes

    QPointer<FaceDetection> m_facedetection;

    ApiClient* m_apiclient;
    QPointer<Contest> m_contest;

    QJsonArray current_contest;
};

#endif // HOME_H
