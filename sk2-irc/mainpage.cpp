#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include "mainpage.h"
#include "ui_mainpage.h"

#pragma comment(lib, "ws2_32.lib")

std::string username;

mainpage::mainpage(SOCKET newfd, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mainpage)
{
    ui->setupUi(this);
    ui->currentChannelLabel->setText("Main channel");
    fd = newfd;
    getUser();
}

mainpage::~mainpage()
{
    delete ui;
}

void mainpage::getUser()
{
    std::ifstream userinfo;
    userinfo.open("userinfo.txt");
    userinfo>>username;
    userinfo.close();
}

void mainpage::on_sendButton_clicked()
{
    //read new message from user
    QString newMessage = ui->textEdit->toPlainText();
    //format for sending
    newMessage = "S0 " + newMessage;
    //convert to const char*
    QByteArray ba = newMessage.toUtf8();
    const char *toSend = ba.data();
    //send
    std::cout<<strlen(toSend)<<std::endl;
    send(fd,toSend,strlen(toSend)+1, 0);

    ui->textEdit->clear();
}

void mainpage::on_channelButton_clicked()
{

}

void mainpage::onMessageReceived(QString username, QString message)
{
    QString newMessage = username + ": " + message;
    ui->textBrowser->append(newMessage);
}

