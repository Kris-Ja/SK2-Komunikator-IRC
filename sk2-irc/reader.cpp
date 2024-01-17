#include <QThread>
#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <openssl/ssl.h>
#include "reader.h"
#include "mainpage.h"

#define MAX_CMD_SIZE 300

Reader::Reader(SOCKET newfd, SSL_CTX* newctx, SSL* newssl)
{
    fd = newfd;
    ctx = newctx;
    ssl = newssl;
}

void Reader::run()
{
    char buf[MAX_CMD_SIZE];
    int chat_id,i;
    std::string username, message, channelName;

    while(1)
    {
        if(_read(ssl,buf,sizeof(buf))==0) break; //server closed

        printf("%s\n",buf);
        fflush(stdout);

        //get chat id
        chat_id = buf[1]- '0';
        i = 2;
        while(buf[i] != ' ' && buf[i] != '\0') chat_id = chat_id*10 + buf[i++] - '0';
        i++;

        switch(buf[0])
        {
            case 'S': //new message
                username = "";
                message = "";

                while(buf[i] != '\n') username+=buf[i++];
                i++;
                while(buf[i] != '\0') message+=buf[i++];

                emit newMessage(chat_id, QString::fromStdString(username),QString::fromStdString(message));
                break;
            case 'J': //user joined chat
                username = "";
                while(buf[i] != '\0') username+=buf[i++];

                emit userJoined(chat_id, QString::fromStdString(username));
                break;
            case 'U': //user joined chat before us
                username = "";
                while(buf[i] != '\0') username+=buf[i++];

                emit userJoined(chat_id, QString::fromStdString(username));
                break;
            case 'E': //user left chat
                username = "";
                while(buf[i] != '\0') username+=buf[i++];

                emit userLeft(chat_id, QString::fromStdString(username));
                break;
            case 'L': //list channel
                channelName = "";
                while(buf[i] != '\0') channelName+=buf[i++];

                emit newChannel(chat_id, QString::fromStdString(channelName), 0);
                break;
            case 'D': //channel closed
                emit channelDeleted(chat_id);
                break;
            case 'C': //response after creating channel
                channelName = "";
                while(buf[i] != '\0') channelName+=buf[i++];

                emit newChannel(chat_id, QString::fromStdString(channelName), 1);
                break;
        }
    }
    std::cout<<"it's so over"<<std::endl;
}

int Reader::_read(SSL* ssl, char *buf, int bufsize)
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
        int i = SSL_read(ssl, buf+l, bufsize);
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


