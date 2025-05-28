// lineedit.h
#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>

class LineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit LineEdit(QWidget* parent = nullptr);

    static const QString NICK_PLH;
    static const QString UNUM_PLH;
    static const QString PWD_PLH;
    static const QString PWD_AGAIN_PLH;
    static const QString SEARCH_PLH;

protected:
    void focusInEvent(QFocusEvent* event);
    void focusOutEvent(QFocusEvent* event);

private:
    void restore();
};

#endif
