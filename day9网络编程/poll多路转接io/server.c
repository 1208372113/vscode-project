#include <stdio.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include "./include/wrap.h"

#define SERV_PORT 8888
#define SERV_IP "127.0.0.1"
#define OPEN_MAX 4096

int main(int argc, char const *argv[])
{
    int i, n;
    int lfd, cfd;
    struct sockaddr_in clie_addr, serv_addr;
    socklen_t clie_addr_len;
    struct pollfd client[OPEN_MAX];
    char str[INET_ADDRSTRLEN], buf[BUFSIZ];

    //创建socket
    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    //端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //绑定port+ip
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    //设置监听
    Listen(lfd, 128);

    //初始化poll数组
    bzero(client, sizeof(client));
    for (i = 0; i < OPEN_MAX; i++)
    {
        client[i].fd = -1;
    }

    //设置监控文件描述符集合
    client[0].fd = lfd;
    client[0].events = POLLIN;

    //设置下标

    int maxi = 0, nready;

    while (1)
    {
        //监听是否有客户端连接
        nready = poll(client, maxi + 1, -1);

        //连接事件监听
        if (client[0].revents & POLLIN)
        {
            clie_addr_len = sizeof(clie_addr);

            cfd = Accept(lfd, (struct sockaddr *)&clie_addr,
                         &clie_addr_len);

            //放入数组中最新空位
            for (i = 0; i < OPEN_MAX; i++)
            {
                if (client[i].fd < 0)
                {
                    client[i].fd = cfd;
                    printf("client%d port:%d ip%s\n", i,
                           ntohs(clie_addr.sin_port),
                           inet_ntop(AF_INET, &clie_addr.sin_addr,
                                     str, sizeof(str)));
                    break;
                }
            }

            if (i == OPEN_MAX)
            {
                printf("客户已满\n");
            }

            client[i].events = POLLIN;

            if (i > maxi)
            {
                maxi = i;
            }
            if (--nready == 0)
            {
                continue;
            }
        }

        //判断哪个文件描述符事件
        for (i = 1; i < OPEN_MAX; i++)
        {
            if (client[i].fd == -1)
            {
                continue;
            }
            if (client[i].revents & POLLIN)
            {
                bzero(buf,sizeof(buf));
                n = Read(client[i].fd, buf, sizeof(buf));
                if (n<0)
                {
                    if (errno == ECONNRESET)
                    {
                        printf("client%d aborted connection\n", i);
                        Close(client[i].fd);
                        client[i].fd = -1;
                        client[i].events = 0;
                    }
                    else
                    {
                        perr_exit("read error");
                    }
                }
                else if (n == 0)
                {
                    printf("client[%d] closed connection\n", i);
                    Close(client[i].fd);
                    client[i].fd = -1;
                    client[i].events = 0;

                }
                else
                {
                    Write(client[i].fd, buf, n);
                }
                if (--nready == 0)
                {
                    break;
                }
                
            }
        }
    }

    return 0;
}
