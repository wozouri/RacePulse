#ifndef DIALOG_H
#define DIALOG_H

#include <qdialog.h>
#include <qpainter.h>

#include <QWidget>

namespace Ui
{
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget* parent = nullptr);
    ~Dialog();

    void paintEvent(QPaintEvent* event);

    void setText(QString text);

    // page : notice choice
    void setNotice();
    void setChoice();

    void confEdit();

private slots:
    void on_pu_sure_clicked();

    void on_pu_yes_clicked();

    void on_pu_no_clicked();

signals:

private:
    Ui::Dialog* ui;
};

#endif // DIALOG_H
