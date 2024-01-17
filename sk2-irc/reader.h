#ifndef READER_H
#define READER_H

#include <QThread>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>

class Reader : public QThread
{
    Q_OBJECT

public:
    Reader(SOCKET, SSL_CTX*, SSL*);

signals:
    void newMessage(int chat_id, QString username, QString message);
    void newChannel(int chat_id, QString channelName, bool joined);
    void userJoined(int chat_id, QString username);
    void channelDeleted(int chat_id);
    void userLeft(int chat_id, QString username);

protected:
    void run();

private:
    int _read(SSL*, char*, int);
    SOCKET fd;
    SSL_CTX* ctx;
    SSL* ssl;
};

#endif // READER_H
