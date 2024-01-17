#ifndef READER_H
#define READER_H

#include <QThread>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

class Reader : public QThread
{
    Q_OBJECT

public:
    Reader(SOCKET);

signals:
    void newMessage(int chat_id, QString username, QString message);
    void newChannel(int chat_id, QString channelName, bool joined);
    void userJoined(int chat_id, QString username);
    void channelDeleted(int chat_id);
    void userLeft(int chat_id, QString username);

protected:
    void run();

private:
    int _read(int, char*, int);
    SOCKET fd;
};

#endif // READER_H
