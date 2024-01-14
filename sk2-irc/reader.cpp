#include <QThread>
#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include "reader.h"
#include "mainpage.h"

#define MAX_CMD_SIZE 300

Reader::Reader(SOCKET newfd)
{
    fd = newfd;
}

void Reader::run()
{
    char buf[MAX_CMD_SIZE];

    while(1)
    {
        if(_read(fd,buf,sizeof(buf))==0) break;

        printf("%s\n",buf);
        fflush(stdout);

        //get chat id
        int chat_id = buf[1]- '0';
        int i = 2;
        while(buf[i] != ' ') chat_id = chat_id*10 + buf[i++] - '0';

        i++;
        std::string username = "";
        std::string message = "";

        while(buf[i] != '\n') username+=buf[i++];
        i++;
        while(buf[i] != '\0') message+=buf[i++];

        emit newMessage(QString::fromStdString(username),QString::fromStdString(message));
    }
    std::cout<<"it's so over"<<std::endl;
}

int Reader::_read(int fd, char *buf, int bufsize)
{
    static int last_size = 0;
    static char last_buf[MAX_CMD_SIZE];
    memcpy(buf, last_buf, last_size);
    int l = last_size;
    bufsize -= last_size;
    for(int i=0; i<l; i++) if(buf[i]=='\0'){
            memcpy(last_buf, buf+i+1, l-i-1);
            last_size = l-i-1;
            return i+1;
        }
    do {
        int i = recv(fd, buf+l, bufsize, 0);
        if(i == 0) return 0;
        bufsize -= i;
        l += i;
        for(int j=l-i; j<l; j++) if(buf[j]=='\0'){
                memcpy(last_buf, buf+j+1, l-j-1);
                last_size = l-j-1;
                return j+1;
            }
    } while (bufsize>0);
    return l;
}

