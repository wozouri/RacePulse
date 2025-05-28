#ifndef AVATARWIDGET_H
#define AVATARWIDGET_H

#include <qpainterpath.h>

#include <QLabel>

#include "qevent.h"
#include "qpainter.h"

class AvatarWidget : public QLabel
{
    Q_OBJECT
private:
    QPixmap originalPixmap;
    QPixmap scaledPixmap;
    QPointF centerPoint;
    qreal zoomFactor = 1.0;
    qreal minZoom = 1.0;
    qreal maxZoom = 3.0;
    QPoint lastMousePos;
    bool isMoving = false;

protected:
    void wheelEvent(QWheelEvent* event) override;

    void updateScaledPixmap();

    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

public:
    explicit AvatarWidget(QWidget* parent);

    QPixmap getCroppedAvatar();

    void setOriginalPixmap(const QPixmap& pixmap);
};

#endif // AVATARWIDGET_H
