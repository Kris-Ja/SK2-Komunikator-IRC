#ifndef NAMECHANNEL_H
#define NAMECHANNEL_H

#include <QWidget>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

namespace Ui {
class NameChannel;
}

class NameChannel : public QWidget
{
    Q_OBJECT

public:
    explicit NameChannel(SOCKET, QWidget *parent = nullptr);
    ~NameChannel();

private slots:
    void on_pushButton_clicked();

private:
    Ui::NameChannel *ui;
    SOCKET fd;
};

#endif // NAMECHANNEL_H
