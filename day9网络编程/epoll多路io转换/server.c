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
#include "wrap.h"

#define SERV_PORT 8888
#define SERV_IP "127.0.0.1"
#define OPEN_MAX 4096

int main(int argc, char const *argv[])
{
    int lfd, cfd;
    int n, num;
    struct epoll_event event = {0}, evts[10];
    struct sockaddr_in clie_addr, serv_addr;
    socklen_t clie_addr_len;
    char buf[BUFSIZ];
    char str[INET_ADDRSTRLEN];
    bzero(evts, sizeof(evts));

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_family = AF_INET;
    //serv_addr.sin_addr.s_addr=inet_addr(SERV_IP);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    Listen(lfd, 10);

    int epfd = epoll_create(10);
    if (epfd == -1)
    {
        perr_exit("epoll_create error");
    }

    event.events = EPOLLIN;
    event.data.fd = lfd;
    int err = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &event);
    if (err == -1)
    {
        perror("ctl error");
        exit(1);
    }

    while (1)
    {
        n = epoll_wait(epfd, evts, 10, -1);
        if (n == -1)
        {
            perr_exit("epoll_wait error");
        }

        for (int i = 0; i < n; i++)
        {
            if (!(evts[i].events & EPOLLIN))
            {
                continue;
            }

            if (evts[i].data.fd == lfd)
            {
                clie_addr_len = sizeof(clie_addr);
                cfd = Accept(lfd, (struct sockaddr *)&clie_addr,
                             &clie_addr_len);

                event.data.fd = cfd;
                event.events = EPOLLIN;
                err = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd,
                                &event);
                if (err == -1)
                {
                    perror("ctl error");
                    exit(1);
                }

                printf("client%d 上线 port:%d ip:%s\n", cfd-4,
                       ntohs(clie_addr.sin_port),
                       inet_ntop(AF_INET, &clie_addr.sin_addr,
                                 str, sizeof(str)));
            }
            else
            {
                num = Read(evts[i].data.fd, buf, sizeof(buf));
                if (num < 0)
                {
                    printf("client%d 故障\n", evts[i].data.fd-4);
                    err=epoll_ctl(epfd, EPOLL_CTL_DEL, evts[i].data.fd, &event);
                    if (err == -1)
                    {
                        perr_exit("ctl error");
                    }
                    
                    Close(evts[i].data.fd);
                    break;
                }
                else if (num == 0)
                {
                    bzero(buf,sizeof(buf));
                    printf("client%d断开连接\n", evts[i].data.fd-4);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, evts[i].data.fd, &event);
                    Close(evts->data.fd);
                    break;
                }
                fprintf(stdout,"client%d:%s\n",evts[i].data.fd-4,buf);
                Write(evts[i].data.fd, buf, num);
            }
        }
    }

    return 0;
}
