#include <iostream>
#include <fstream>
#include "mainpage.h"
#include "ui_mainpage.h"

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

