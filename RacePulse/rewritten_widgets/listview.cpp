#include "listview.h"

#include "qevent.h"
#include "qpainter.h"

ListView::ListView(QWidget* parent)
    : QListView(parent)
{
    this->setItemDelegate(new RichDelegate());
}

void ListView::mousePressEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid())
    {
        emit sigItemSelected(index.row());
    }

    QListView::mousePressEvent(event);
}

void RichDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // 保存绘制状态
    painter->save();

    // 根据选中状态改变背景
    if (option.state & QStyle::State_Selected)
    {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    // 绘制Logo
    QPixmap logo = index.data(Qt::DecorationRole).value<QPixmap>();
    QRect logoRect = QRect(option.rect.left() + 10,
                           option.rect.top() + 10,
                           50, 50); // 固定Logo大小
    painter->drawPixmap(logoRect, logo);

    // 比赛名称
    QFont nameFont = painter->font();
    nameFont.setBold(true);
    nameFont.setPointSize(10);
    painter->setFont(nameFont);

    QRect nameRect = QRect(logoRect.right() + 10,
                           option.rect.top() + 10,
                           option.rect.width() - logoRect.width() - 20,
                           30);
    painter->drawText(nameRect,
                      Qt::AlignLeft | Qt::AlignTop,
                      index.data(Qt::DisplayRole).toString());

    // 开始时间
    QFont timeFont = painter->font();
    timeFont.setPointSize(8);
    painter->setFont(timeFont);
    painter->setPen(Qt::gray);

    QRect timeRect = QRect(nameRect.left(),
                           nameRect.bottom() + 5,
                           nameRect.width(),
                           20);
    painter->drawText(timeRect,
                      Qt::AlignLeft | Qt::AlignTop,
                      index.data(Qt::UserRole).toString()); // 假设时间存储在UserRole中

    // 恢复绘制状态
    painter->restore();
}

// 配套的sizeHint方法
QSize RichDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize(option.rect.width(), 70); // 固定高度
}

QWidget* RichDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return nullptr;
}
