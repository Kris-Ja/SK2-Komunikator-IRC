#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <QWidget>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <QListWidget>
#include <QTextBrowser>
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui {
class mainpage;
}
QT_END_NAMESPACE

class mainpage : public QWidget
{
    Q_OBJECT

public:
    mainpage(SOCKET, QWidget *parent = nullptr);
    ~mainpage();

public slots:
    void onMessageReceived(int chat_id, QString username, QString message);
    void onChannelCreated(int chat_id, QString channelName, bool joined);
    void onUserJoined(int chat_id, QString username);
    void onChannelDeleted(int chat_id);
    void onUserLeft(int chat_id, QString username);

private slots:
    void on_sendButton_clicked();
    void on_channelButton_clicked();

    void on_channelList_itemDoubleClicked(QListWidgetItem *item);

    void on_channelList_itemClicked(QListWidgetItem *item);

private:
    Ui::mainpage *ui;
    void getUser();
    SOCKET fd;
    int currentChat;
    QListWidget* userLists[30];
    QTextBrowser* chats[30];
};
#endif // MAINPAGE_H
