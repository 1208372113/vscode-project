#include <stdio.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include "./include/wrap.h"

#define SERV_PORT 8888
#define SERV_IP "127.0.0.1"

int main(int argc, char const *argv[])
{

    int listenfd, cfd;
    int n; //读出字数

    struct sockaddr_in clie_addr, serv_addr;
    char buf[BUFSIZ];

    //1创建socket
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    //设置端口复用
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt,
               sizeof(int));

    //绑定ip port
    bzero(&serv_addr, sizeof(clie_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind(listenfd, (struct sockaddr *)&serv_addr,
         sizeof(serv_addr));

    //设置监听
    Listen(listenfd, 20);

    //将lfd加入readfds集合
    fd_set readfds,tmpfds; //定义文件描述符集变量
    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);

    //设置监测文件描述符范围
    int maxfd = listenfd + 1;

    //定义实时监听数个数
    int nready;

    while (1)
    {
        tmpfds=readfds;
        nready = select(maxfd + 1, &tmpfds, NULL, NULL, NULL);
        if (nready < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }

        //有客户端连接请求到来
        if (FD_ISSET(listenfd, &tmpfds))
        {
            //接收新的客服端请求
            cfd = Accept(listenfd, NULL, NULL);

            //将cfd加入到readfds
            FD_SET(cfd, &readfds);

            //修改内核监控文件描述符范围
            if (maxfd < cfd)
            {
                maxfd = cfd;
            }

            if (--nready == 0)
            {
                continue;
            }

            FD_CLR(listenfd,&tmpfds);
        }

        //有客户端数据发来
        for (int i = listenfd; i <= maxfd; i++)
        {
            if (FD_ISSET(i, &tmpfds))
            {
                while (1)
                {
                    //读数据
                    n = Read(i, buf, sizeof(buf));
                    if (n < 0)
                    {
                        printf("%d\n",__LINE__);
                        FD_CLR(i, &tmpfds);
                        break;
                    }
                    if (n==0)
                    {
                        printf("client%d closed connection\n",i);
                    }
                    

                    //write发送应答数据
                    Write(i, buf, n);
                }
            }
        }
    }
    Close(listenfd);
    return 0;
}