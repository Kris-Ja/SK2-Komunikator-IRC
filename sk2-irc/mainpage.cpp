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
    ui->setupUi(this);

    //initialise widgets for channels
    for(int i=0; i<30; i++)
    {
        QListWidget* newListWidget = new QListWidget(this);
        userLists[i] = newListWidget;
        ui->gridLayout->addWidget(userLists[i],1,2);
        userLists[i]->setMaximumWidth(150);
        userLists[i]->hide();

        QTextBrowser* newTextBrowser = new QTextBrowser(this);
        chats[i] = newTextBrowser;
        ui->gridLayout->addWidget(chats[i],1,1);
        chats[i]->hide();
    }

    ui->currentChannelLabel->setText("0 - main channel");
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

void mainpage::onChannelCreated(int chat_id, QString channelName, bool joined)
{
    //display channel name
    char* stringID;
    channelName = QString::fromUtf8(itoa(chat_id, stringID, 10)) + " - " + channelName;
    if(joined == 0) channelName = channelName + " +";
    ui->channelList->addItem(channelName);
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
    QList<QListWidgetItem *> items = ui->channelList->findItems(QString::number(chat_id)+" ", Qt::MatchStartsWith);
    if (items.size() > 0) {
        int number = ui->channelList->row(items[0]);
        QListWidgetItem* deleted = ui->channelList->takeItem(number);
    }
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

void mainpage::on_channelList_itemDoubleClicked(QListWidgetItem *item) //leave chat
{
    QString name = item->text();
    if(name.endsWith('+')) return; //user is not on chanel

    //get chat id
    int chat_id = name.front().unicode() - '0';
    int i = 1;
    while(name.at(i) != ' ') chat_id = chat_id*10 + name.at(i++).unicode() - '0';
    std::cout<<"current chat to be deleted "<<chat_id<<std::endl;

    //add + from channel name
    QList<QListWidgetItem *> items = ui->channelList->findItems(name, Qt::MatchExactly);
    if (items.size() > 0) {
        int number = ui->channelList->row(items[0]);
        QListWidgetItem* deleted = ui->channelList->takeItem(number);
        ui->channelList->addItem(name+" +");
    }
    //send message to server
    QString newMessage = "E" + QString::number(chat_id);
    QByteArray ba = newMessage.toUtf8();
    const char *toSend = ba.data();
    //send
    std::cout<<strlen(toSend)<<std::endl;
    send(fd,toSend,strlen(toSend)+1, 0);

    //hide chat if current visible and show main
    if(chat_id == currentChat)
    {
        ui->currentChannelLabel->setText("0 - main channel");

        chats[currentChat]->hide();
        userLists[currentChat]->hide();

        currentChat = 0;

        chats[0]->show();
        userLists[0]->show();
    }
}

void mainpage::on_channelList_itemClicked(QListWidgetItem *item) //join and view chat
{
    QString name = item->text();

    //get chat id
    int chat_id = name.front().unicode() - '0';
    int i = 1;
    while(name.at(i) != ' ') chat_id = chat_id*10 + name.at(i++).unicode() - '0';

    //check if user already joined channel
    if(name.endsWith('+'))
    {
        //send message to server
        QString newMessage = "J" + QString::number(chat_id);
        QByteArray ba = newMessage.toUtf8();
        const char *toSend = ba.data();
        //send
        std::cout<<strlen(toSend)<<std::endl;
        send(fd,toSend,strlen(toSend)+1, 0);

        //remove + from channel name
        QList<QListWidgetItem *> items = ui->channelList->findItems(name, Qt::MatchExactly);
        if (items.size() > 0) {
            int number = ui->channelList->row(items[0]);
            QListWidgetItem* deleted = ui->channelList->takeItem(number);
        }
        name.chop(2);
        ui->channelList->addItem(name);

        //clear channel widgets
        chats[chat_id]->clear();
        userLists[chat_id]->clear();
    }

    //set channel as visible
    chats[currentChat]->hide();
    userLists[currentChat]->hide();
    ui->currentChannelLabel->setText(name);
    currentChat = chat_id;

    chats[chat_id]->show();
    userLists[chat_id]->show();
}

