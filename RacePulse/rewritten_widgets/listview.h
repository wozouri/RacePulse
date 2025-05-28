#ifndef LISTVIEW_H
#define LISTVIEW_H

#include <QListView>
#include <QStyledItemDelegate>

class ListView : public QListView
{
    Q_OBJECT
public:
    explicit ListView(QWidget* parent = nullptr);
    void mousePressEvent(QMouseEvent* event);

signals:
    void sigItemSelected(int index);
};

class RichDelegate : public QStyledItemDelegate
{
public:
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;
};

#endif // LISTVIEW_H
