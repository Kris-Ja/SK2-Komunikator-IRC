#ifndef NAMECHANNEL_H
#define NAMECHANNEL_H

#include <QWidget>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>

namespace Ui {
class NameChannel;
}

class NameChannel : public QWidget
{
    Q_OBJECT

public:
    explicit NameChannel(SOCKET, SSL_CTX*, SSL*, QWidget *parent = nullptr);
    ~NameChannel();

private slots:
    void on_pushButton_clicked();

private:
    Ui::NameChannel *ui;
    SOCKET fd;
    int _write(SSL* ssl, char *buf, int len);
    SSL_CTX* ctx;
    SSL* ssl;
};

#endif // NAMECHANNEL_H
