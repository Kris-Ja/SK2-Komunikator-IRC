#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <openssl/ssl.h>
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
    void openMainWindow(mainpage*, QString);
    void saveUsername(QString);
    void establishConnection(QString,QString);
    int _write(SSL* ssl, char *buf, int len);
};

#endif // LOGIN_H
