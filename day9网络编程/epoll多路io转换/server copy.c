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
    int n;
    struct epoll_event event, evts[10];
    struct sockaddr_in clie_addr, serv_addr;
    socklen_t clie_addr_len;
    char buf[BUFSIZ];

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    Listen(lfd, 10);

    int epfd = epoll_creat(10);

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
        int n = epoll_wait(epfd, evts, 20, -1);
        for (int i = 0; i < n; i++)
        {
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

                printf("client%d 上线 port:%d ip:%s\n", cfd,
                       ntohs(clie_addr.sin_port),
                       inet_addr(&clie_addr.sin_addr));
            }
            else
            {
                n=Read(evts[i].data.fd,buf,sizeof(buf));
                if (n<0)
                {
                    printf("client%d 断开连接\n",evts[i].data.fd);
                    break;
                }
                else if(n==0)
                {
                    printf("client%d发送完成\n",evts[i].data.fd);
                    break;
                }
                
                Write(evts[i].data.fd,buf,n);
            }
            
        }
    }

    return 0;
}
