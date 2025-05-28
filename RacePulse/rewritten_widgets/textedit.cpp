#include "textedit.h"

TextEdit::TextEdit(QWidget* parent)
    : QTextEdit(parent)
{
}

void TextEdit::focusOutEvent(QFocusEvent* event)
{
    QTextCursor cursor = this->textCursor();
    cursor.clearSelection();     // 取消选择
    this->setTextCursor(cursor); // 更新光标状态
    // 基类的事件处理函数自带取消选择，不过有白痕，具体实现看个人选择

    QTextEdit::focusOutEvent(event);
}
