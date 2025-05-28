#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>

class TextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit TextEdit(QWidget* parent = nullptr);
    void focusOutEvent(QFocusEvent* event);
};

#endif // TEXTEDIT_H
