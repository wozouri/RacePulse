#include "label.h"

#include <Register.h>

#include <QFileDialog>

#include "custom_controls/avatarcrop.h"
#include "qpainter.h"
#include "qpainterpath.h"
#include "qtimer.h"

Label_avatar::Label_avatar(QWidget* parent)
    : QLabel(parent)
{
}

void Label_avatar::mousePressEvent(QMouseEvent* event)
{
    QString filePath = QFileDialog::
        getOpenFileName(this,
                        "选择头像",
                        "",
                        "图像文件 (*.png *.jpg *.jpeg *.bmp)");

    if (!filePath.isEmpty())
    {
        AvatarCrop* cropDialog = new AvatarCrop(filePath, this);
        connect(cropDialog, &AvatarCrop::cutOk, qobject_cast<Register*>(this->parentWidget()), &Register::receAvator);
        cropDialog->exec();
        cropDialog->deleteLater();
    }
}

Label_time::Label_time(QWidget* parent)
    : QLabel(parent), m_timeTimer(new QTimer(this)), m_dateTimer(new QTimer(this)), m_countdownTimer(new QTimer(this)), m_remainingSeconds(0), m_isCountdownActive(false)
{
    // 设置定时器，每秒更新一次
    connect(m_timeTimer, &QTimer::timeout, this, &Label_time::showCurrentTime);
    connect(m_dateTimer, &QTimer::timeout, this, &Label_time::showCurrentDate);
    connect(m_countdownTimer, &QTimer::timeout, this, &Label_time::updateCountdown);
}

void Label_time::setShowMode(ShowMode mode)
{
    switch (mode)
    {
    case Current_Time:
    {
        if (m_dateTimer->isActive())
            m_dateTimer->stop();
        m_timeTimer->start(1000);
        showCurrentTime();
        break;
    }
    case Current_Date:
    {
        if (m_timeTimer->isActive())
            m_timeTimer->stop();
        m_dateTimer->start(60000);
        showCurrentDate(); // 不然要60s后才会显示
        break;
    }
    default:
        break;
    }
}

void Label_time::showCurrentTime()
{
    // 显示当前时分秒
    QTime currentTime = QTime::currentTime();
    QString timeText = currentTime.toString("hh:mm:ss");
    setText(timeText);
}

void Label_time::showCurrentDate()
{
    // 显示当前年月日
    QDate currentDate = QDate::currentDate();
    QString dateText = currentDate.toString("yyyy-MM-dd");
    setText(dateText);
}

void Label_time::setCountdownTarget(const QDateTime& targetDateTime)
{
    // 获取当前时间
    QDateTime currentDateTime = QDateTime::currentDateTime();

    // 计算与目标时间的差值（秒）
    qint64 totalSeconds = currentDateTime.secsTo(targetDateTime);

    // 如果目标时间已过去，允许倒计时显示负数
    m_remainingSeconds = totalSeconds;

    // 标记倒计时活动状态
    m_isCountdownActive = true;
    m_countdownTimer->start(1000); // 每秒更新
}

void Label_time::setSecondsCountdown(int totalSeconds)
{
    m_remainingSeconds = totalSeconds;
    m_isCountdownActive = true;
    m_countdownTimer->start(1000);
}

void Label_time::updateCountdown()
{
    if (m_isCountdownActive)
    {
        // 更新剩余时间
        m_remainingSeconds--;

        // 计算正负时间
        qint64 absoluteSeconds = qAbs(m_remainingSeconds);
        int days = absoluteSeconds / 86400;
        int hours = (absoluteSeconds % 86400) / 3600;
        int minutes = (absoluteSeconds % 3600) / 60;
        int seconds = absoluteSeconds % 60;

        // 根据时间正负决定显示格式
        QString countdownText;
        if (m_remainingSeconds >= 0)
        {
            if (days > 0)
            {
                countdownText = QString("%1d %2:%3:%4")
                                    .arg(days)
                                    .arg(hours, 2, 10, QChar('0'))
                                    .arg(minutes, 2, 10, QChar('0'))
                                    .arg(seconds, 2, 10, QChar('0'));
            }
            else
            {
                countdownText = QString("%1:%2:%3")
                                    .arg(hours, 2, 10, QChar('0'))
                                    .arg(minutes, 2, 10, QChar('0'))
                                    .arg(seconds, 2, 10, QChar('0'));
            }
        }
        else
        {
            if (days > 0)
            {
                countdownText = QString("- %1d %2:%3:%4")
                                    .arg(days)
                                    .arg(hours, 2, 10, QChar('0'))
                                    .arg(minutes, 2, 10, QChar('0'))
                                    .arg(seconds, 2, 10, QChar('0'));
            }
            else
            {
                countdownText = QString("- %1:%2:%3")
                                    .arg(hours, 2, 10, QChar('0'))
                                    .arg(minutes, 2, 10, QChar('0'))
                                    .arg(seconds, 2, 10, QChar('0'));
            }
        }

        // 更新显示
        setText(countdownText);

        // 停止计时器仅在用户手动停止时
    }
}

Label_face::Label_face(QWidget* parent)
    : QLabel(parent)
{
    setMinimumSize(200, 200); // 设置控件的最小尺寸
}

void Label_face::paintEvent(QPaintEvent* event)
{
    QLabel::paintEvent(event); // 保留 QLabel 的原有绘制逻辑

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 设置画笔和画刷
    QPen pen(Qt::blue, 2); // 蓝色边框，宽度为2
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // 计算头部的矩形区域
    int headWidth = width() / 3;
    int headHeight = height() * 2 / 3;
    QRect headRect((width() - headWidth) / 2, height() / 8, headWidth, headHeight);

    // 绘制头部轮廓（椭圆）
    painter.drawEllipse(headRect);

    // 绘制肩膀的曲线
    QPoint leftShoulder(headRect.left() - headWidth / 2, headRect.bottom() + headHeight / 4);
    QPoint rightShoulder(headRect.right() + headWidth / 2, headRect.bottom() + headHeight / 4);
    QPoint controlPoint(width() / 2, headRect.bottom());

    QPainterPath shoulderPath;
    shoulderPath.moveTo(leftShoulder);
    shoulderPath.quadTo(controlPoint, rightShoulder);
    painter.drawPath(shoulderPath);

    // 绘制水平和垂直对齐线
    QPen alignPen(Qt::red, 1, Qt::DashLine); // 红色虚线
    painter.setPen(alignPen);

    // 水平对齐线（头部中线）
    painter.drawLine(headRect.left(), headRect.center().y(), headRect.right(), headRect.center().y());

    // 垂直对齐线（中心线）
    painter.drawLine(headRect.center().x(), 0, headRect.center().x(), height());
}
