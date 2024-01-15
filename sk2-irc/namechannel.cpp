#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include "namechannel.h"
#include "ui_namechannel.h"

NameChannel::NameChannel(SOCKET newfd, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NameChannel)
{
    ui->setupUi(this);
    fd = newfd;
}

NameChannel::~NameChannel()
{
    delete ui;
}

void NameChannel::on_pushButton_clicked()
{
    //read channel name from user
    QString newChannel = ui->textEdit->toPlainText();
    //format for sending
    newChannel = "C" + newChannel;
    //convert to const char*
    QByteArray ba = newChannel.toUtf8();
    const char *toSend = ba.data();
    printf("%s\n",toSend);
    //send
    std::cout<<strlen(toSend)<<std::endl;
    send(fd,toSend,strlen(toSend)+1, 0);

    this->hide();
}

