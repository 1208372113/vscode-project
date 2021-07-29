#include <stdio.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "./include/wrap.h"

#define SERV_PORT 8888
#define SERV_IP "127.0.0.1"

int main(int argc, char const *argv[])
{
    int i, n;
    int lfd, cfd;
    struct sockaddr_in clie_addr, serv_addr;
    socklen_t clie_addr_len;
    char client[FD_SETSIZE], str[INET_ADDRSTRLEN], buf[BUFSIZ];

    //将存放文件描述符表置为-1
    for (i = 0; i < FD_SETSIZE; i++)
    {
        client[i] = -1;
    }

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
    Listen(lfd, 20);

    //设置监控文件描述符集合
    fd_set readset, tmpset;
    FD_ZERO(&readset);
    FD_SET(lfd, &readset);

    //数组下标
    int maxi = -1, maxfd = lfd, nready;

    while (1)
    {

        //select设置监视资源
        tmpset = readset;
        nready = select(maxfd + 1, &tmpset, NULL, NULL, NULL);
        if (nready < 0)
        {
            perror("select error");
            exit(1);
        }

        //判断是否连接服务器
        if (FD_ISSET(lfd, &tmpset))
        {
            bzero(&clie_addr, sizeof(clie_addr));
            clie_addr_len = sizeof(clie_addr);
            cfd = Accept(lfd, (struct sockaddr *)&clie_addr, &clie_addr_len);

            //设置非阻塞属性
            int flag;
            fcntl(cfd, F_GETFL);
            flag |= O_NONBLOCK;
            fcntl(cfd, F_SETFL, flag);

            //添加到源文件描述符集
            FD_SET(cfd, &readset);

            //文件描述符分配最新数组
            for (i = 0; i < FD_SETSIZE; i++)
            {
                if (client[i] < 0)
                {
                    client[i] = cfd;
                    printf("client%d 已连接,port:%d,ip:%s\n", i, ntohs(clie_addr.sin_port), inet_ntop(AF_INET, &clie_addr.sin_addr, str, sizeof(str)));
                    break;
                }
            }
            //判断数组元素是否已满
            if (i == FD_SETSIZE)
            {
                fprintf(stderr, "client full\n");
                exit(1);
            }
            //最大元素下标更新
            if (maxi < i)
            {
                maxi = i;
            }
            //最大文件描述符更新
            if (maxfd < cfd)
            {
                maxfd = cfd;
            }
            //清除文件描述符集标记
            FD_CLR(cfd, &tmpset);
            //判断事件是否都解决
            if (--nready == 0)
            {
                continue;
            }
        }

        for (i = 0; i <= maxfd; i++)
        {
            if (client[i] == -1)
            {
                continue;
            }
            if (FD_ISSET(client[i], &tmpset))
            {
                // while (1)
                // {
                FD_CLR(client[i], &tmpset);
                bzero(buf, BUFSIZ);
                n = Read(client[i], buf, BUFSIZ);
                if (n < 0)
                {
                    perror("Read error");
                    Close(client[i]);
                    FD_CLR(client[i], &readset);
                    break;
                }

                if (n == 0)
                {
                    printf("client%d断开连接\n", i);
                    Close(client[i]);
                    FD_CLR(client[i], &readset);
                    break;
                }

                printf("client%d已转发\n", i);
                Write(client[i], buf, n);
                // }
            }
        }
    }

    return 0;
}
