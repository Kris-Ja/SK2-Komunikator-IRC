#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <QWidget>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

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
    void onMessageReceived(QString username, QString message);

private slots:
    void on_sendButton_clicked();
    void on_channelButton_clicked();

private:
    Ui::mainpage *ui;
    void getUser();
    SOCKET fd;
};
#endif // MAINPAGE_H
