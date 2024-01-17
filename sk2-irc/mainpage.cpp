#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <QListWidget>
#include <QTextBrowser>
//#include <QListWidgetItem>
#include "mainpage.h"
#include "ui_mainpage.h"
#include "namechannel.h"

#pragma comment(lib, "ws2_32.lib")

std::string username;

mainpage::mainpage(SOCKET newfd, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mainpage)
{
    //initialise channels
    for(int i=0; i<30; i++)
    {
        QListWidget* newListWidget = new QListWidget(this);
        userLists[i] = newListWidget;
        userLists[i]->hide();

        QTextBrowser* newTextBrowser = new QTextBrowser(this);
        chats[i] = newTextBrowser;
        chats[i]->hide();
    }

    ui->setupUi(this);
    ui->currentChannelLabel->setText("Main channel");
    ui->channelList->addItem("0 - main channel");
    fd = newfd;
    currentChat = 0;
    userLists[0] = ui->userList;
    chats[0] = ui->textBrowser;
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
    newMessage = "S" + QString::number(currentChat) + " " + newMessage;
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
    NameChannel* nameChannel = new NameChannel(fd);
    nameChannel->setAttribute(Qt::WA_DeleteOnClose);
    nameChannel->show();
}

void mainpage::onMessageReceived(int chat_id, QString username, QString message)
{
    QString newMessage = username + ": " + message;
    chats[chat_id]->append(newMessage);
}

void mainpage::onChannelCreated(int chat_id, QString channelName)
{
    //display channel name
    char* stringID;
    channelName = QString::fromUtf8(itoa(chat_id, stringID, 10)) + " - " + channelName;
    ui->channelList->insertItem(chat_id, channelName);
    //QListWidgetItem *newItem = new QListWidgetItem;
    //newItem->setText(channelName);
    //ui->channelList->addItem(newItem);
}

void mainpage::onUserJoined(int chat_id, QString username)
{
    //chats[chat_id]->append(username + " joined the chat");
    userLists[chat_id]->addItem(username);
}

void mainpage::onChannelDeleted(int chat_id)
{
    //clear channel view
    userLists[chat_id]->clear();
    chats[chat_id]->clear();

    //delete from channel list
    QListWidgetItem* deleted = ui->channelList->takeItem(chat_id);
}

void mainpage::onUserLeft(int chat_id, QString username)
{
    chats[chat_id]->append(username + " left the chat");
    QList<QListWidgetItem *> items = userLists[chat_id]->findItems(username, Qt::MatchExactly);
    if (items.size() > 0) {
        int number = userLists[chat_id]->row(items[0]);
        QListWidgetItem* deleted = userLists[chat_id]->takeItem(number);
    }
}


void mainpage::on_channelList_itemDoubleClicked(QListWidgetItem *item)
{
    ui->currentChannelLabel->setText(item->text());
    int chat_id = ui->channelList->row(item);

    chats[currentChat]->hide();
    userLists[currentChat]->hide();

    currentChat = chat_id;

    chats[chat_id]->show();
    userLists[chat_id]->show();
}

