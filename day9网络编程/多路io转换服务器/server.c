#include <stdio.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "wrap.h"

#define SERV_PORT 8888
#define SERV_IP "127.0.0.1"

int main(int argc, char const *argv[])
{
    int i, j, n, maxi;
    int nready, client[FD_SETSIZE];
    int maxfd, listenfd, connfd, sockfd;
    char buf[BUFSIZ], str[INET_ADDRSTRLEN];

    struct sockaddr_in clie_addr, serv_addr;
    socklen_t clie_addr_len;
    fd_set rset, allset; //allset保存

    //1创建socket
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    //设置端口复用
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    //绑定ip port
    bzero(&serv_addr, sizeof(clie_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    //设置监听
    Listen(listenfd, 20);

    maxfd = listenfd;

    maxi = -1;
    for (int i = 0; i < FD_SETSIZE; i++)
    {
        client[i] = -1;
    }

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    while (1)
    {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0)
        {
            perror("select error");
        }

        //判断是否有新连接
        if (FD_ISSET(listenfd, &rset))
        {
            clie_addr_len = sizeof(clie_addr);
            //连接
            connfd = Accept(listenfd, (struct sockaddr *)&clie_addr, &clie_addr_len);
            printf("received from %s at PORT %d\n",
                   inet_ntop(AF_INET, &clie_addr.sin_addr,
                             str, sizeof(str)),
                   ntohs(clie_addr.sin_port));

            //放到数组最新一个位置
            for (i = 0; i < FD_SETSIZE; i++)
            {
                if (client[i] < 0)
                {
                    client[i] = connfd;
                    break;
                }
            }

            //判断是否已经装满文件描述符表
            if (i == FD_SETSIZE)
            {
                fprintf(stderr, "too many clients\n");
                exit(1);
            }

            //调整最大文件描述符
            FD_SET(connfd, &allset);
            if (connfd > maxfd)
            {
                maxfd = connfd;
            }

            //
            if (i > maxi)
            {
                maxi = i; //保证maxi存的是最后一个元素下标
            }

            if (--nready == 0)
            {
                continue;
            }
        }

        //判断读数据
        for (int i = 0; i < maxi; i++)
        {
            if ((sockfd = client[i]) < 0)
            {
                continue;
            }
            if (FD_ISSET(sockfd, &rset))
            {
                if ((n = Read(sockfd, buf, sizeof(buf))) == 0)
                {
                    Close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                }
                else if (n > 0)
                {
                    for (int j = 0; j < n; j++)
                    {
                        buf[j] = toupper(buf[j]);
                    }
                    sleep(10);
                    Write(sockfd, buf, n);
                }
                if (--nready == 0)
                {
                    break;
                }
            }
        }
    }

    Close(listenfd);
    return 0;
}
