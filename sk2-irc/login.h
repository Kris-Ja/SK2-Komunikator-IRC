#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include "mainpage.h"

namespace Ui {
class login;
}

class login : public QWidget
{
    Q_OBJECT

public:
    explicit login(QWidget *parent = nullptr);
    ~login();

private slots:
    void on_confirmButton_clicked();

private:
    Ui::login *ui;
    void openMainWindow(QString);
    void saveUsername(QString);
};

#endif // LOGIN_H
