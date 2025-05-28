#include "contest.h"

#include "ui_contest.h"

Contest::Contest(QJsonObject contest_info, ApiClient* apiclient, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Contest)
    , m_apiclient(apiclient)
    , m_contest_info(ContestInfo::fromJson(contest_info))
{
    ui->setupUi(this);
    page_ctpwd_init();
}

Contest::~Contest()
{
    delete ui;
}

void Contest::page_ctpwd_init()
{
    ui->lab_pwdwrong->clear(); // 用hide会导致布局变化，这种占位的方式比较好
    ui->lab_ctname->setText(m_contest_info.contest_name);
    ui->lab_countdown->setCountdownTarget(m_contest_info.start_time);
    if (m_contest_info.contest_password == "")
    {
        ui->stackedWidget->setCurrentWidget(ui->page_ctsign);
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->page_ctpwd);
    }

    setVal();
}

void Contest::page_ctmain_init()
{
}

void Contest::setVal()
{
    QRegularExpression regExp("^[a-zA-Z0-9]{1,12}$"); // 设置验证器
    Contest::validator = new QRegularExpressionValidator(regExp, this);
    ui->linee_ctpwd->setValidator(validator);
}

void Contest::on_face_check_success()
{
    ui->stackedWidget->setCurrentWidget(ui->page_ctmain);
}

void Contest::on_pu_ctpwd_clicked()
{
    QString cnpwd = ui->linee_ctpwd->text(); // 用户输入的比赛密码

    if (cnpwd == m_contest_info.contest_password)
    {
        // 恢复 QLineEdit 样式并隐藏错误提示
        ui->linee_ctpwd->setStyleSheet(
            "font: 12pt 'Microsoft YaHei UI';"
            "border: 1px solid rgba(0, 0, 0, 0.3);"
            "border-radius: 10px;"
            "color:grey;");
        ui->lab_pwdwrong->clear();
        // 密码正确，切换页面
        QTimer::singleShot(100, [this]() { // 加点小延迟
            ui->stackedWidget->setCurrentWidget(ui->page_ctsign);
        });
    }
    else if (cnpwd == "")
    {
        // 密码为空
        ui->lab_pwdwrong->setText("密码不能为空");
        ui->lab_pwdwrong->setAlignment(Qt::AlignRight | Qt::AlignVCenter); // 靠右 + 垂直居中
        ui->lab_pwdwrong->setStyleSheet("color: red; font-weight: bold;");

        // 设置 QLineEdit 边框为红色
        ui->linee_ctpwd->setStyleSheet(
            "font: 12pt 'Microsoft YaHei UI';"
            "border: 1px solid rgba(0, 0, 0, 0.3);"
            "border-radius: 10px;"
            "color:grey;"
            "border: 2px solid red;");
    }
    else
    {
        // 密码错误，设置 QLineEdit 为红色边框
        ui->lab_pwdwrong->setText("密码错误");
        ui->lab_pwdwrong->setAlignment(Qt::AlignRight | Qt::AlignVCenter); // 靠右 + 垂直居中
        ui->lab_pwdwrong->setStyleSheet("color: red; font-weight: bold;");

        // 设置 QLineEdit 边框为红色
        ui->linee_ctpwd->setStyleSheet(
            "font: 12pt 'Microsoft YaHei UI';"
            "border: 1px solid rgba(0, 0, 0, 0.3);"
            "border-radius: 10px;"
            "color:grey;"
            "border: 2px solid red;");
    }
}

void Contest::on_linee_ctpwd_textChanged(const QString& arg1)
{
    // 恢复 QLineEdit 样式并隐藏错误提示
    ui->linee_ctpwd->setStyleSheet(
        "font: 12pt 'Microsoft YaHei UI';"
        "border: 1px solid rgba(0, 0, 0, 0.3);"
        "border-radius: 10px;"
        "color:grey;");
    ui->lab_pwdwrong->clear();
}

void Contest::on_pu_sign_clicked()
{
    if (m_facedetection.isNull())
    {
        // 人脸检测窗口
        m_facedetection = new FaceDetection(m_apiclient, "check", this); // 父对象为 Contest
        m_facedetection->setWindowFlags(Qt::Window);                     // 设置为独立窗口
    }

    connect(m_facedetection, &FaceDetection::sigFaceCheckSuccess, this, &Contest::on_face_check_success);
    m_facedetection->setWindowModality(Qt::ApplicationModal); // 设置为应用程序模态
    m_facedetection->show();
    m_facedetection->activateWindow();
}
