#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include "mainpage.h"
#include "ui_mainpage.h"

#pragma comment(lib, "ws2_32.lib")

std::string username;

mainpage::mainpage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mainpage)
{
    ui->setupUi(this);
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
    ui->textBrowser->append(QString::fromStdString(username));
    QString newMessage = ui->textEdit->toPlainText();
    ui->textEdit->clear();
    ui->textBrowser->append(newMessage);
}


void mainpage::on_testButton_clicked()
{
    Sleep(5000);
    ui->textBrowser->append(QString::fromStdString(username));
    QString newMessage = ui->textEdit->toPlainText();
    ui->textEdit->clear();
    ui->textBrowser->append(newMessage);
}

