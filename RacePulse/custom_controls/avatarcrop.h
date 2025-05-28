#ifndef AVATARCROP_H
#define AVATARCROP_H

#include <qlabel.h>

#include <QDialog>

namespace Ui
{
class AvatarCrop;
}

class AvatarCrop : public QDialog
{
    Q_OBJECT

public:
    explicit AvatarCrop(const QString& imagePath, QWidget* parent = nullptr);
    ~AvatarCrop();

    void paintEvent(QPaintEvent* event);

    void setImage(QPixmap& newImage);
private slots:
    void on_pu_crop_clicked();

signals:
    void cutOk(const QPixmap& pixmap); // 裁剪结束

private:
    Ui::AvatarCrop* ui;
    QPixmap image;
};

#endif // AVATARCROP_H
