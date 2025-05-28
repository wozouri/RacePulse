#include "lineedit.h"

const QString LineEdit::NICK_PLH = "请输入昵称";
const QString LineEdit::UNUM_PLH = "请输入账号";
const QString LineEdit::PWD_PLH = "请输入密码";
const QString LineEdit::PWD_AGAIN_PLH = "请再次输入密码";
const QString LineEdit::SEARCH_PLH = "请输入比赛名称/比赛ID";

LineEdit::LineEdit(QWidget* parent)
    : QLineEdit(parent)
{
    restore();
}
void LineEdit::focusInEvent(QFocusEvent* event)
{
    if (text() == NICK_PLH || text() == UNUM_PLH ||
        text() == PWD_PLH || text() == PWD_AGAIN_PLH)
    {
        setText("");
        setStyleSheet(
            "font: 12pt 'Microsoft YaHei UI';"
            "border: 1px solid rgba(0,0,0,0.3);"
            "border-radius: 10px;"
            "color: grey;");
    }
    else if (text() == SEARCH_PLH)
    {
        setText("");
        setStyleSheet(
            "font: 10pt 'Microsoft YaHei UI';"
            "border-radius: 3px;"
            "background:rgb(243, 245, 246);"
            "color:grey;"
            "padding: 5px;");
    }
    if (this->objectName() == "linee_password" || this->objectName() == "linee_password_again")
        this->setEchoMode(QLineEdit::Password);
    setClearButtonEnabled(true);
    QLineEdit::focusInEvent(event);
}

void LineEdit::focusOutEvent(QFocusEvent* event) // 失去焦点处理事件
{
    setClearButtonEnabled(false);
    restore();
    QLineEdit::focusOutEvent(event);
}

void LineEdit::restore()
{
    if (text() == NICK_PLH || text() == UNUM_PLH ||
        text() == PWD_PLH || text() == PWD_AGAIN_PLH)
    {
        setStyleSheet(
            "font: 12pt 'Microsoft YaHei UI'; "
            "border: 1px solid rgba(0, 0, 0, 0.3); "
            "border-radius: 10px; "
            "color: grey;");
    }
    else if (text() == SEARCH_PLH)
    {
        setStyleSheet(
            "font: 10pt 'Microsoft YaHei UI';"
            "border-radius: 3px;"
            "background:rgb(243, 245, 246);"
            "color:grey;"
            "padding: 5px;");
    }
    if (!text().isEmpty())
        return;

    setEchoMode(Normal);
    if (objectName() == "linee_password")
        setText(PWD_PLH);
    else if (objectName() == "linee_usernum")
        setText(UNUM_PLH);
    else if (objectName() == "linee_password_again")
        setText(PWD_AGAIN_PLH);
    else if (objectName() == "linee_nickname")
        setText(NICK_PLH);
    else if (objectName() == "linee_search")
        setText(SEARCH_PLH);
}
