#include "avatarwidget.h"

#include "custom_controls/avatarcrop.h"

AvatarWidget::AvatarWidget(QWidget* parent)
    : QLabel(parent), originalPixmap(), scaledPixmap(), centerPoint(0, 0), zoomFactor(1.0), minZoom(1.0), maxZoom(3.0)
{
    // setAlignment(Qt::AlignCenter);
    // setScaledContents(false);
    // setMouseTracking(true);
}
void AvatarWidget::wheelEvent(QWheelEvent* event)
{
    // 控制缩放
    qreal step = event->angleDelta().y() > 0 ? 0.1 : -0.1;
    zoomFactor = qBound(minZoom, zoomFactor + step, maxZoom);
    updateScaledPixmap();
}
void AvatarWidget::updateScaledPixmap()
{
    // 等比例缩放并居中
    scaledPixmap = originalPixmap.scaled(
        width() * zoomFactor,
        height() * zoomFactor,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    update();
}

void AvatarWidget::paintEvent(QPaintEvent* event)
{
    // 创建一个新的QPixmap作为画布，大小与当前控件一致
    QPixmap resultPixmap(size());
    resultPixmap.fill(Qt::transparent); // 设置透明背景

    // 在这个pixmap上进行绘制
    QPainter painter(&resultPixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 创建圆形裁剪区域
    QPainterPath path;
    path.addEllipse(rect());
    painter.setClipPath(path);

    // 绘制缩放和移动后的图像
    QPointF offset = QPointF(width() / 2, height() / 2) + centerPoint;
    painter.drawPixmap(offset - QPointF(scaledPixmap.width() / 2, scaledPixmap.height() / 2),
                       scaledPixmap);

    painter.end();

    qobject_cast<AvatarCrop*>(this->parentWidget())->setImage(resultPixmap);

    QPainter painter1(this);
    painter1.drawPixmap(0, 0, resultPixmap);
}

void AvatarWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        isMoving = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void AvatarWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        isMoving = false;
        setCursor(Qt::ArrowCursor);
    }
}

// void AvatarWidget::mouseMoveEvent(QMouseEvent* event)
// {
//     if (isMoving && (event->buttons() & Qt::LeftButton))
//     {
//         QPoint delta = event->pos() - lastMousePos;
//         centerPoint += QPointF(delta.x(), delta.y());
//         lastMousePos = event->pos();
//         update();
//     }
// }
void AvatarWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (isMoving && (event->buttons() & Qt::LeftButton))
    {
        QPoint delta = event->pos() - lastMousePos;
        QPointF newCenterPoint = centerPoint + QPointF(delta.x(), delta.y());

        // 设置固定的移动范围（例如控件尺寸的1/10）
        float maxMove = qMin(width(), height()) / 10.0f * zoomFactor * zoomFactor;

        // 限制移动范围
        newCenterPoint.setX(qBound(-maxMove, newCenterPoint.x(), maxMove));
        newCenterPoint.setY(qBound(-maxMove, newCenterPoint.y(), maxMove));

        centerPoint = newCenterPoint;
        lastMousePos = event->pos();
        update();
    }
}
QPixmap AvatarWidget::getCroppedAvatar()
{
    QPixmap croppedPixmap(width(), height());
    croppedPixmap.fill(Qt::transparent);

    QPainter painter(&croppedPixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 创建圆形遮罩
    QPainterPath path;
    path.addEllipse(croppedPixmap.rect());
    painter.setClipPath(path);

    // 绘制裁剪区域
    QPointF offset((width() - scaledPixmap.width()) / 2.0,
                   (height() - scaledPixmap.height()) / 2.0);
    painter.drawPixmap(offset, scaledPixmap);

    return croppedPixmap;
}

void AvatarWidget::setOriginalPixmap(const QPixmap& pixmap)
{
    originalPixmap = pixmap;
    scaledPixmap = pixmap;
    centerPoint = QPointF(0, 0);
    zoomFactor = 1.0;

    scaledPixmap = pixmap.scaled(
        width() * zoomFactor,
        height() * zoomFactor,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    update();
}
