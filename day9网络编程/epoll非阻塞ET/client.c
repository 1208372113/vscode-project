//epoll非阻塞单客户通信
#include <sys/epoll.h>
#include <stdio.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include "wrap.h"

#define SERV_PORT 8888
#define SERV_IP "127.0.0.1"
#define OPEN_MAX 4096

int main(int argc, char const *argv[])
{
    int lfd;
    int i;
    struct sockaddr_in serv_addr;
    char buf[10];
    char ch = 'a';

    //socket
    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    //connect
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);
    Connect(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    while (1)
    {
        for (i = 0; i < 5; i++)
        {
            buf[i] = ch;
        }
        buf[i - 1] = '\n';
        ch++;
        for (i = 5; i < 10; i++)
        {
            buf[i] = ch;
        }
        buf[i - 1] = '\n';
        ch++;
        Write(lfd, buf, sizeof(buf));
        sleep(5);
    }

    return 0;
}
