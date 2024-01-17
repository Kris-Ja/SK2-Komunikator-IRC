#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <openssl/ssl.h>
#include "namechannel.h"
#include "ui_namechannel.h"

NameChannel::NameChannel(SOCKET newfd, SSL_CTX* newctx, SSL* newssl, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NameChannel)
{
    ui->setupUi(this);
    fd = newfd;
    ctx = newctx;
    ssl = newssl;
}

NameChannel::~NameChannel()
{
    delete ui;
}

int NameChannel::_write(SSL* ssl, char *buf, int len){
    int l = len;
    while (len > 0) {
        int i = SSL_write(ssl, buf, len);
        if(i == 0) return 0;
        len -= i;
        buf += i;
    }
    return l;
}

void NameChannel::on_pushButton_clicked()
{
    //read channel name from user
    QString newChannel = ui->textEdit->toPlainText();
    //format for sending
    newChannel = "C" + newChannel;
    //convert to const char*
    QByteArray ba = newChannel.toUtf8();
    char *toSend = ba.data();
    printf("%s\n",toSend);
    //send
    std::cout<<strlen(toSend)<<std::endl;
    _write(ssl,toSend,strlen(toSend)+1);

    this->hide();
}

