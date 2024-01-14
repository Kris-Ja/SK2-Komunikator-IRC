#include "login.h"
#include "mainpage.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    //run application
    QApplication a(argc, argv);
    login l;
    l.show();
    return a.exec();
}
