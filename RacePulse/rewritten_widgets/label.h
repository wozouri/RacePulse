#ifndef LABEL_H
#define LABEL_H

#include <QLabel>

class Label_avatar : public QLabel // 供点击的头像（register页面）
{
    Q_OBJECT
public:
    explicit Label_avatar(QWidget* parent = nullptr);

    void mousePressEvent(QMouseEvent* event); // 点击头像更换头像
};

class Label_time : public QLabel
{
    Q_OBJECT

public:
    explicit Label_time(QWidget* parent = nullptr);

    enum ShowMode
    {
        Current_Time,
        Current_Date
    };

    void setShowMode(ShowMode mode);

    // 显示当前时分秒
    void showCurrentTime();

    // 显示当前年月日
    void showCurrentDate();

    // 设置秒倒计时
    void setSecondsCountdown(int totalSeconds);

    // 按照目标时间设置倒计时
    void setCountdownTarget(const QDateTime& targetDateTime);
private slots:

    // 更新倒计时
    void updateCountdown();

private:
    QTimer* m_timeTimer; // 用于更新时间显示的定时器
    QTimer* m_dateTimer;
    QTimer* m_countdownTimer; // 用于倒计时的定时器

    int m_remainingSeconds;   // 剩余倒计时秒数
    bool m_isCountdownActive; // 是否正在倒计时
};

class Label_face : public QLabel
{
    Q_OBJECT

public:
    explicit Label_face(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};
#endif // LABEL_H
