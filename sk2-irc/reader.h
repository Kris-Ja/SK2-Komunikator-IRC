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
    void newMessage(QString username, QString message);

protected:
    void run();

private:
    int _read(int, char*, int);
    SOCKET fd;
};

#endif // READER_H
