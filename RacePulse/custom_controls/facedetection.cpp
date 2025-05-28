#include "facedetection.h"

#include <qmessagebox.h>

#include <QBuffer>

#include "custom_controls/dialog.h"
#include "qevent.h"
#include "ui_facedetection.h"

FaceDetection::FaceDetection(ApiClient* apiclient, QString mode, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::FaceDetection)
    , m_apiclient(apiclient)
    , m_mode(mode)
    , captureSession(new QMediaCaptureSession(this))
    , camera(new QCamera(this))
    , videoSink(new QVideoSink(this))
    , timer(new QTimer(this))
    , mediaPlayer(new QMediaPlayer(this))

{
    ui->setupUi(this);

    setWindow();

    m_apiclient->connectToServer();

    const auto videoInputs = QMediaDevices::videoInputs();
    if (videoInputs.isEmpty())
    {
        QMessageBox::critical(this, "Error", "No camera found!");
        return;
    }
    // 使用第一个可用的摄像头
    camera->setCameraDevice(videoInputs.first());
    // camera = new QCamera(videoInputs.first(), this);
    // 设置输入输出设备
    captureSession->setCamera(camera);
    captureSession->setVideoOutput(videoSink);

    camera->start();
    if (!camera->isActive())
    {
        qDebug() << "Camera failed to start!";
        QMessageBox::critical(this, "Error", "Failed to start camera.");
    }
    connect(videoSink, &QVideoSink::videoFrameChanged, this, &FaceDetection::onNewFrame);

    connect(timer, &QTimer::timeout, this, &FaceDetection::processFrame);
    timer->start(100); // 每100ms传一次

    connect(m_apiclient, &ApiClient::dataReceived, this, &FaceDetection::recvFace);
}

FaceDetection::~FaceDetection()
{
    if (timer && timer->isActive())
    {
        timer->stop(); // 停止定时器
    }
    if (camera && camera->isActive())
    {
        camera->stop(); // 关闭摄像头
    }
    delete ui;
}

void FaceDetection::setWindow()
{
    QIcon icon(":/images/logo.png");
    setWindowIcon(icon);
    setWindowTitle("赛搏");
}

void FaceDetection::displayerImage(const QImage image)
{
    if (image.isNull())
    {
        qDebug() << "Empty image in displayerImage!";
        return;
    }
    QSize labelSize = ui->lab_face->size();
    QPixmap pixmap = QPixmap::fromImage(image).scaled(
        labelSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

    ui->lab_face->setPixmap(pixmap);
}

void FaceDetection::processFrame()
{
    if (currentFrame.isNull())
    {
        qDebug() << "No frame captured!";
        return;
    }

    // 播放当前摄像头画面
    displayerImage(currentFrame);
}

void FaceDetection::onNewFrame(const QVideoFrame& frame)
{
    if (!frame.isValid())
    {
        qDebug() << "Invaild video frame!";
        return;
    }
    QImage image = frame.toImage();
    if (image.isNull())
    {
        qDebug() << "Failed to convert frame to QImage!";
        return;
    }

    // 横向翻转一下
    currentFrame = image.mirrored(true, false);
}

void FaceDetection::sendFace(const QImage image)
{
    // 将 QImage 转换为 PNG 格式的字节数组
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    // 将字节数组转换为 Base64 编码字符串
    QString base64Image = byteArray.toBase64();
    QString usernum = m_apiclient->getUsernum();

    if (usernum == nullptr)
    {
        qDebug() << " usernum not found ";
        return;
    }

    // 创建 QJsonObject，并将 Base64 编码字符串存入其中
    QJsonObject jsonObject;
    jsonObject["tag"] = "face";
    jsonObject["mode"] = m_mode;
    jsonObject["usernum"] = usernum;
    jsonObject["face"] = base64Image;

    // 调用 sendJsonRequest 发送
    m_apiclient->sendJsonRequest(jsonObject);
}

void FaceDetection::recvFace(const QJsonObject& recvJson)
{
    if (recvJson["tag"] != "face")
        return;

    if (recvJson["mode"] == "save")
    {
        if (recvJson["result"] == "success")
        {
            // 验证通过
            Dialog msgBox(this);
            msgBox.setNotice();
            msgBox.setText("保存成功");
            msgBox.exec();
            emit sigFaceSaveSuccess();
            this->close();
        }
        else if (recvJson["result"] == "fail")
        {
            Dialog msgBox(this);
            msgBox.setNotice();
            msgBox.setText("保存失败:\n" + recvJson["reason"].toString());
            msgBox.exec();
            ui->pu_action->setEnabled(true);
        }
    }
    else if (recvJson["mode"] == "check")
    {
        if (recvJson["result"] == "success")
        {
            // 验证通过
            Dialog msgBox(this);
            msgBox.setNotice();
            msgBox.setText("验证通过");
            msgBox.exec();
            emit sigFaceCheckSuccess();
            this->close();
        }
        else if (recvJson["result"] == "fail")
        {
            Dialog msgBox(this);
            msgBox.setNotice();
            msgBox.setText("验证失败:\n" + recvJson["reason"].toString());
            msgBox.exec();
            ui->pu_action->setEnabled(true);
        }
    }
    else if (recvJson["mode"] == "modify")
    {
        if (recvJson["result"] == "success")
        {
            // 验证通过
            Dialog msgBox(this);
            msgBox.setNotice();
            msgBox.setText("人脸信息修改成功");
            msgBox.exec();
            emit sigFaceModifySuccess();
            this->close();
        }
        else if (recvJson["result"] == "fail")
        {
            Dialog msgBox(this);
            msgBox.setNotice();
            msgBox.setText("人脸信息修改失败:\n" + recvJson["reason"].toString());
            msgBox.exec();
            ui->pu_action->setEnabled(true);
        }
    }
}

void FaceDetection::closeEvent(QCloseEvent* event)
{
    if (camera && camera->isActive())
    {
        camera->stop();
    }
    if (timer && timer->isActive())
    {
        timer->stop();
    }
    event->accept();
    this->deleteLater(); // 确保对象被销毁
}

void FaceDetection::setMode(QString mode)
{
    m_mode = mode;
    if (mode == "check")
    {
        ui->pu_action->setText("点击验证");
    }
    else if (mode == "save")
    {
        ui->pu_action->setText("点击保存");
    }
    else if (mode == "modify")
    {
        ui->pu_action->setText("点击修改(将删除原有人脸数据)");
    }
}

void FaceDetection::on_pu_action_clicked()
{
    if (m_mode == "modify")
    {
        Dialog msgBox(this);
        msgBox.setChoice();
        msgBox.setText("操作会删除原有的人脸数据");
        // 以模态方式运行对话框，等待用户选择
        if (msgBox.exec() == QDialog::Rejected) // 假设 Dialog::ChoiceNo 是表示用户选择“否”的返回值
        {
            qDebug() << "return ";
            return; // 用户选择了“否”，直接退出函数
        }
    }

    sendFace(currentFrame);
    // 先关闭发送，避免用户多次发送
    ui->pu_action->setEnabled(false);
}
