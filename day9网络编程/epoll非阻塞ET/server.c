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

    // event.events = EPOLLIN;
    event.events=EPOLLIN|EPOLLET;
    clie_addr_len=sizeof(clie_addr);
    cfd=Accept(lfd,(struct sockaddr*)&clie_addr,&clie_addr_len);
    printf("client%d port:%d ip:%s\n",cfd,ntohs(SERV_PORT),
        inet_ntop(AF_INET,&clie_addr.sin_addr,str,sizeof(str)));
    
    // int flag=fcntl(cfd,F_GETFL);
    // flag|=O_NONBLOCK;
    // fcntl(cfd,F_SETFL,flag);
    
    event.data.fd = cfd;
    int err = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &event);
    if (err == -1)
    {
        perror("ctl error");
        exit(1);
    }

    while (1)
    {
        n=epoll_wait(epfd,evts,10,-1);
        printf("n:%d\n",n);
        if (evts[0].data.fd==cfd)
        {
            while ((num=Read(cfd,buf,5))>0)
            {
                Write(STDOUT_FILENO,buf,num);
            }
        }   
    }
    return 0;
}