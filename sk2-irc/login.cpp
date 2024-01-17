#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include "login.h"
#include "ui_login.h"
#include "mainpage.h"
#include "reader.h"

#pragma comment(lib, "ws2_32.lib")

WSADATA wd;
SOCKET fd;
sockaddr_in sa;
hostent* he;
char buf[1024];

login::login(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::login)
{
    ui->setupUi(this);
}

login::~login()
{
    delete ui;
}

void login::on_confirmButton_clicked()
{
    QString username = ui->usernameTextEdit->toPlainText();
    QString address = ui->addressTextEdit->toPlainText();
    QString port = ui->portTextEdit->toPlainText();

    establishConnection(address, port);
    saveUsername(username);

    send(fd,username.toStdString().c_str(),sizeof(username.toStdString().c_str()), 0);

    Reader* reader = new Reader(fd);
    mainpage* mainPage = new mainpage(fd);
    connect(reader,Reader::newMessage,mainPage,mainpage::onMessageReceived);
    connect(reader,Reader::newChannel,mainPage,mainpage::onChannelCreated);
    connect(reader,Reader::userJoined,mainPage,mainpage::onUserJoined);
    connect(reader,Reader::channelDeleted,mainPage,mainpage::onChannelDeleted);
    connect(reader,Reader::userLeft,mainPage,mainpage::onUserLeft);
    reader->start();

    openMainWindow(mainPage,username);
}

void login::saveUsername(QString username)
{
    std::string user = username.toStdString();
    std::string path = QCoreApplication::applicationDirPath().toStdString();

    std::ofstream userinfo;
    userinfo.open("userinfo.txt");
    userinfo<<user+"\n";
    userinfo.close();
}

void login::establishConnection(QString address, QString port)
{
    std::cout<<address.toStdString()<<" "<<port.toStdString()<<std::endl;

    WSAStartup(MAKEWORD(2, 2), &wd);
    he = gethostbyname(address.toStdString().c_str());
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sa.sin_family = AF_INET;
    memcpy(&sa.sin_addr.s_addr, he->h_addr, he->h_length);
    sa.sin_port = htons(port.toInt());
    ::connect(fd, (sockaddr*)&sa,sizeof(sa));
    std::cout<<"connected"<<std::endl;
}

void login::openMainWindow(mainpage* mainPage, QString username)
{
    this->hide();
    mainPage->setAttribute(Qt::WA_DeleteOnClose);
    mainPage->show();
}

