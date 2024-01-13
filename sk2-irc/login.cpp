#include <iostream>
#include <fstream>
#include "login.h"
#include "ui_login.h"
#include "mainpage.h"

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
    saveUsername(username);
    openMainWindow(username);
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

void login::openMainWindow(QString username)
{
    this->hide();
    mainpage* huh = new mainpage;
    huh->setAttribute(Qt::WA_DeleteOnClose);
    huh->show();
}

