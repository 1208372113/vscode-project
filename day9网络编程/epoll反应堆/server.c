#include <stdio.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/epoll.h>
#include <errno.h>
#include "./include/wrap.h"

#define SERV_PORT 8000
#define MAX_EVENTS 1024
#define BUFLEN 4096
#define SERV_IP "127.0.0.1"

struct myevent_s
{
    int fd;                                           //监听的文件描述符
    int events;                                       //监听对应事件
    void *arg;                                        //泛型参数
    void (*call_back)(int fd, int events, void *arg); //回调函数
    int status;                                       //是否再监听:1在树上;0不在
    char buf[BUFLEN];
    int len;
    long last_active; //加入红黑树根的时间值
};

int g_efd;                                 //树根
struct myevent_s g_events[MAX_EVENTS + 1]; //自定义结构体列表

void initlistensocket(int efd, unsigned short port);
void recvdata(int fd, int events, void *arg);
void senddata(int fd, int events, void *arg);
void eventdel(int fd, struct myevent_s *ev);
void eventset(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg);
void eventadd(int efd, int events, struct myevent_s *ev);
void acceptconn(int lfd, int events, void *arg);

void recvdata(int fd, int events, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;

    int len;
    len = recv(fd, ev->buf, sizeof(ev->buf), 0);

    eventdel(g_efd, ev);

    if (len > 0)
    {
        ev->len = len;
        ev->buf[len] = '\0';
        printf("client%d:%s\n", fd, ev->buf);

        eventset(ev, fd, senddata, ev);
        eventadd(g_efd, EPOLLOUT, ev);
    }
    else if (len == 0)
    {
        Close((ev->fd));
        printf("client%d pos[%ld],close\n", fd, ev - g_events);
    }
    else
    {
        Close((ev->fd));
        printf("recv[fd=%d] error:%s", fd, strerror(errno));
    }
    return;
}
void senddata(int fd, int events, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    struct myevent_s tmp = *ev;

    int len;

    len = send(fd, ev->buf, strlen(ev->buf), 0);
    if (len > 0)
    {
        printf(("send[fd=%d], [%d]%s\n"), fd, len, ev->buf);
        eventdel(fd, &tmp);
        eventset(&tmp, fd, recvdata, ev);
        eventadd(g_efd, EPOLLIN, ev);
    }
    else
    {
        Close(ev->fd);
        eventdel(g_efd, ev);
        printf("send[fd=%d] error%s", ev->fd, strerror(errno));
    }
    return;
}
void eventdel(int fd, struct myevent_s *ev)
{
    struct epoll_event epv = {0, {0}};
    if (ev->status == 0)
    {
        return;
    }

    epv.data.ptr = ev;
    ev->status = 0;
    epoll_ctl(g_efd, EPOLL_CTL_DEL, ev->fd, &epv);
}
void eventset(struct myevent_s *ev, int fd,
              void (*call_back)(int, int, void *), void *arg)
{
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    ev->len = 0;
    ev->last_active = time(NULL);

    return;
}

void eventadd(int efd, int events, struct myevent_s *ev)
{
    struct epoll_event epv = {0, {0}};
    int op;
    epv.data.ptr = ev;
    epv.events = ev->events = events;

    if (ev->status == 1)
    {
        op = EPOLL_CTL_MOD;
    }
    else
    {
        op = EPOLL_CTL_ADD;
        ev->status = 1;
    }

    if (epoll_ctl(g_efd, op, ev->fd, &epv) < 0)
    {
        printf("events add failed fd=%d,events=%d\n",
               ev->fd, ev->events);
    }
    else
    {
        printf("events add Finish fd=%d,events=%d\n",
               ev->fd, ev->events);
    }

    return;
}
void acceptconn(int lfd, int events, void *arg)
{
    struct sockaddr_in cin;
    socklen_t len = sizeof(cin);
    int cfd, i;

    cfd = Accept(lfd, (struct sockaddr *)&cin, &len);

    do
    {
        //找到离0最近的未被占用的槽
        for (i = 0; i < MAX_EVENTS; i++)
        {
            if (g_events[i].status == 0)
            {
                break;
            }
        }
        if (i == MAX_EVENTS)
        {
            printf("max connect limit:%d\n", MAX_EVENTS);
            break;
        }

        int flag = 0;
        if ((flag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0)
        {
            perror("fcntl fail");
            break;
        }

        //设置事件结构体
        eventset(&g_events[i], cfd, recvdata, &g_events[i]);

        //挂到树上
        eventadd(g_efd, EPOLLIN, &g_events[i]);

    } while (0);

    printf("new connect [%s:%d][time:%ld],pos[%d]\n",
           inet_ntoa(cin.sin_addr), ntohs(cin.sin_port),
           g_events[i].last_active, i);
    ///////////////////////////////////////
}
//
void initlistensocket(int efd, unsigned short port)
{
    //创建监听连接的文件描述符
    int lfd = Socket(AF_INET, SOCK_STREAM, 0);

    //端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //设置非阻塞
    int flag = fcntl(lfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(lfd, F_SETFL, flag);

    //事件对应结构体设置
    eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);

    //连带事件挂树
    eventadd(lfd, EPOLLIN, &g_events[MAX_EVENTS]);

    struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    inet_pton(AF_INET, SERV_IP, &sin.sin_addr);
    Bind(lfd, (struct sockaddr *)&sin, sizeof(sin));
    Listen(lfd, 20);

    return;
}
int main(int argc, char const *argv[])
{
    //如未指定端口则使用默认端口
    unsigned short port = SERV_PORT;
    if (argc == 2)
    {
        port = atoi(argv[1]);
    }

    //建立树得到树根
    g_efd = epoll_create(MAX_EVENTS + 1);
    if (g_efd <= 0)
    {
        printf("create fail\n");
    }

    //初始化监听socket并返回lfd,放入自定义数组末尾
    initlistensocket(g_efd, port);

    //保存实时事件数组
    struct epoll_event events[MAX_EVENTS + 1];
    printf("server running port:%d\n", port);

    int checkpos = 0, i;
    while (1)
    {
        //超时验证
        long now = time(NULL);
        for (i = 0; i < 100; i++, checkpos++)
        {
            if (checkpos == MAX_EVENTS)
            {
                checkpos = 0;
            }
            if (g_events[checkpos].status == 0)
            {
                continue;
            }

            long duration = now - g_events[checkpos].last_active;

            if (duration >= 60)
            {
                Close(g_events[checkpos].fd);
                printf("[fd=%d] timeout\n", g_events[checkpos].fd);
                eventdel(g_efd, &g_events[checkpos]);
            }
        }

        //超时返回0
        int n = epoll_wait(g_efd, events, MAX_EVENTS + 1, 1000);
        if (n < 0)
        {
            perror("epoll_wait error");
            break;
        }

        for (i = 0; i < n; i++)
        {
            struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;

            if ((ev->events & EPOLLIN) && (events[i].events & EPOLLIN)) //读就绪
            {
                ev->call_back(ev->fd, events[i].events, ev->arg);
                events[i].events = EPOLLOUT;
            }
            if ((ev->events & EPOLLOUT) && (events[i].events & EPOLLOUT)) //读就绪
            {
                ev->call_back(ev->fd, events[i].events, ev->arg);
                events[i].events = EPOLLIN;
            }
        }
    }

    return 0;
}
