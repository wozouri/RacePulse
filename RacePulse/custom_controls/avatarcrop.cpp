#include "custom_controls/avatarcrop.h"

#include <qpainter.h>

#include <QPainterPath>

#include "ui_avatarcrop.h"

AvatarCrop::AvatarCrop(const QString& imagePath, QWidget* parent)
    : QDialog(parent), ui(new Ui::AvatarCrop), image(QPixmap(imagePath))
{
    ui->setupUi(this);
    ui->lab_avatar->setOriginalPixmap(image);
}

AvatarCrop::~AvatarCrop()
{
    delete ui;
}

void AvatarCrop::paintEvent(QPaintEvent* event) // 初始化背景
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 反锯齿
    QPixmap pixmap(":/images/bg_b-p.png");
    painter.drawPixmap(0, 0, width(), height(), pixmap);
    QIcon icon(":/images/logo.png");
    setWindowIcon(icon);
    setWindowTitle("赛搏");
}

void AvatarCrop::on_pu_crop_clicked()
{
    emit cutOk(image);
    close();
}

void AvatarCrop::setImage(QPixmap& newImage)
{
    image = newImage;
}
