#include "custom_controls/dialog.h"

#include "ui_dialog.h"

Dialog::Dialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    QIcon icon(":/images/logo.png");
    setWindowIcon(icon);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground); // 设置窗体透明

    confEdit();
    // 窗口居中
    QWidget* topLevelParent = parent ? parent->window() : nullptr;
    if (topLevelParent)
    {
        int x = topLevelParent->x() + (topLevelParent->width() - width()) / 2;
        int y = topLevelParent->y() + (topLevelParent->height() - height()) / 2;
        move(x, y);
    }
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::confEdit()
{
    // 默认使用通知窗口
    ui->stackedWidget->setCurrentWidget(ui->page_Notice);

    ui->lab_notice->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->lab_choice_message->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->lab_notice->setAlignment(Qt::AlignCenter); // 设置文本居中对齐
    ui->lab_notice->setWordWrap(true);             // 启用自动换行
    ui->lab_choice_message->setAlignment(Qt::AlignHCenter);
    ui->lab_choice_message->setWordWrap(true);
}

void Dialog::setText(QString text)
{
    if (ui->stackedWidget->currentWidget() == ui->page_Notice)
    {
        ui->lab_notice->setText(text);
    }
    else if (ui->stackedWidget->currentWidget() == ui->page_Choice)
    {
        ui->lab_choice_message->setText(text);
    }
}

void Dialog::setNotice()
{
    ui->stackedWidget->setCurrentWidget(ui->page_Notice);
}

void Dialog::setChoice()
{
    ui->stackedWidget->setCurrentWidget(ui->page_Choice);
}

void Dialog::paintEvent(QPaintEvent* event) // 窗口背景
{
    // 调用基类方法以处理父类的绘图
    QDialog::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 反锯齿
    // 设置圆角半径
    int radius = 4; // 圆角半径
    // 设置背景颜色
    painter.setBrush(QBrush(QColor(255, 255, 255)));
    // 绘制圆角矩形背景
    painter.drawRoundedRect(rect(), radius, radius);
}

void Dialog::on_pu_sure_clicked()
{
    this->close();
}

void Dialog::on_pu_yes_clicked()
{
    accept();
    this->close();
}

void Dialog::on_pu_no_clicked()
{
    reject();
    this->close();
}
