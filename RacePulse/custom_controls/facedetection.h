#ifndef FACEDETECTION_H
#define FACEDETECTION_H

#include <QCamera>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QWidget>

#include "network_modules/apiclient.h"
#include "qtimer.h"

namespace Ui
{
class FaceDetection;
}

class FaceDetection : public QWidget
{
    Q_OBJECT

public:
    explicit FaceDetection(ApiClient* apiclient, QString mode, QWidget* parent = nullptr);
    ~FaceDetection();

    void setWindow();
    void displayerImage(const QImage image);

    void closeEvent(QCloseEvent* event);

    void setMode(QString mode);
private slots:
    void processFrame();                       // 定时器回调函数处理视频帧
    void onNewFrame(const QVideoFrame& frame); // 视频帧更新时的槽函数

    void sendFace(const QImage image);
    void recvFace(const QJsonObject& recvJson);

    void on_pu_action_clicked();

signals:
    void sigFaceCheckSuccess();
    void sigFaceSaveSuccess();
    void sigFaceModifySuccess();

private:
    Ui::FaceDetection* ui;
    ApiClient* m_apiclient;
    QString m_mode; // mode: save modify check

    QMediaCaptureSession* captureSession; // 媒体捕获会话
    QCamera* camera;                      // 摄像头对象
    QVideoSink* videoSink;                // 视频输出的接收器
    QTimer* timer;                        // 定时器，用于获取一帧摄像头图像
    QMediaPlayer* mediaPlayer;            // 媒体播放器，用于处理音频等内容

    QImage currentFrame; // 存储当前帧的 QImage 对象
};

#endif // FACEDETECTION_H
