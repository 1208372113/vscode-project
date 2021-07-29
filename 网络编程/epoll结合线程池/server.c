#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include "./include/wrap.h"
#include "threadpool.h"

#define SERV_PORT 8000
#define MAX_EVENTS 1024
#define BUFLEN 4096
#define SERV_IP "127.0.0.1"

struct event_s {
  int fd;
  int events;
  void *arg;
  void *(*call_back)(void *arg);
  int status;
  int len;
  char buf[BUFLEN];
  long last_active;
};

int g_efd;
struct event_s myevents[MAX_EVENTS + 1];
void initlistensocket();
void eventset(struct event_s *ev, int fd, void *(*call_back)(void *));
void eventadd(int events, struct event_s *ev);
void eventdel(struct event_s *ev);
void *acceptconn(void *arg);
void *recvdata(void *arg);
void *senddata(void *arg);
void closeconn(struct event_s *ev);

void onlinetime(struct event_s *ev) {
  time_t hour = (time(NULL) - ev->last_active) / 3600;  // 3700 1h1m40s
  time_t min = (time(NULL) - ev->last_active - hour * 3600) / 60;
  time_t sec = time(NULL) - ev->last_active - hour * 3600 - min * 60;
  printf("client[%d]下线 在线时长为%ld时:%ld分:%ld秒\n", ev->fd - 4, hour, min,
         sec);
}
void closeconn(struct event_s *ev) {
  for (int i = 0; i < MAX_EVENTS; i++) {
    if (myevents[i].fd == ev->fd) {
      myevents[i].status = 0;
      printf("client[%d] move tree\n", ev->fd - 4);
      break;
    }
  }
}
void *senddata(void *arg) {
  struct event_s *ev = (struct event_s *)arg;

  for (int i = 0; i < ev->len; i++) {
    ev->buf[i] = toupper(ev->buf[i]);
  }

  int len = Write(ev->fd, ev->buf, ev->len);

  eventdel(ev);
  if (len > 0) {
    printf("send:client[%d]-->[%d]:%s\n", ev->fd - 4, ev->len, ev->buf);
    eventset(ev, ev->fd, recvdata);
    eventadd(EPOLLIN | EPOLLET, ev);
  } else {
    Close(ev->fd);
    printf("send:client[%d]  error[%s]", ev->fd - 4, strerror(errno));
    closeconn(ev);
  }
  return NULL;
}
void *recvdata(void *arg) {
  struct event_s *ev = (struct event_s *)arg;
  bzero(ev->buf, sizeof(ev->buf));
  int len = Read(ev->fd, ev->buf, sizeof(ev->buf));
  ev->len = len;
  eventdel(ev);
  if (len > 0) {
    ev->buf[len] = '\0';
    printf("client[%d]:%s", ev->fd - 4, ev->buf);
    eventset(ev, ev->fd, senddata);
    eventadd(EPOLLOUT | EPOLLET, ev);
  } else if (len == 0) {
    Close(ev->fd);
    onlinetime(ev);
    closeconn(ev);
  } else {
    Close(ev->fd);
    printf("client[%d] error:%s\n", ev->fd - 4, strerror(errno));
    closeconn(ev);
  }
  return NULL;
}
void *acceptconn(void *arg) {
  struct event_s *ev = (struct event_s *)arg;

  int cfd, i;
  struct sockaddr_in clie;
  socklen_t clie_addr_len = sizeof(clie);
  cfd = Accept(ev->fd, (struct sockaddr *)&clie, &clie_addr_len);

  do {
    for (i = 0; i < MAX_EVENTS; i++) {
      if (myevents[i].status == 0) {
        break;
      }
    }
    if (i == MAX_EVENTS) {
      printf("添加连接客户端已满!!,目前客户连接数[%d]\n", MAX_EVENTS);
      break;
    }

    fcntl(cfd, F_SETFL, O_NONBLOCK);
    eventset(&myevents[i], cfd, recvdata);
    myevents[i].last_active = time(NULL);
    eventadd(EPOLLIN | EPOLLET, &myevents[i]);
    printf("new clien[%d],[%d]\n", i + 1, ntohs(clie.sin_port));
    return NULL;
  } while (0);
  printf("connect fail!!\n");

  return NULL;
}
void eventdel(struct event_s *ev) {
  if (ev->status == 0) {
    return;
  }
  epoll_ctl(g_efd, EPOLL_CTL_DEL, ev->fd, NULL);
}
void eventadd(int events, struct event_s *ev) {
  struct epoll_event evt = {0};
  evt.events = ev->events = events;
  evt.data.ptr = ev;
  epoll_ctl(g_efd, EPOLL_CTL_ADD, ev->fd, &evt);
}
void eventset(struct event_s *ev, int fd, void *(*call_back)(void *)) {
  ev->fd = fd;
  ev->status = 1;
  ev->arg = ev;
  ev->call_back = call_back;
}
void initlistensocket() {
  //创建管理连接的socket文件描述符
  int lfd = Socket(AF_INET, SOCK_STREAM, 0);

  //设置端口复用
  int opt = 1;
  setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in serv;
  bzero(&serv, sizeof(serv));
  serv.sin_family = AF_INET;
  serv.sin_port = htons(SERV_PORT);
  serv.sin_addr.s_addr = htonl(INADDR_ANY);
  Bind(lfd, (struct sockaddr *)&serv, sizeof(serv));
  Listen(lfd, 20);

  eventset(&myevents[MAX_EVENTS], lfd, acceptconn);
  eventadd(EPOLLIN | EPOLLET, &myevents[MAX_EVENTS]);
}

int main(int argc, char const *argv[]) {
  //创建树
  g_efd = epoll_create(MAX_EVENTS + 1);
  if (g_efd < 0) {
    perror("树创建失败");
    return -1;
  }

  //创建管理连接lfd socket
  initlistensocket();

  //实时接收事件的数组
  struct epoll_event evts[MAX_EVENTS + 1];

  //创建线程池
  threadpool_t *pool = threadpool_create(3, 100, 100);
  printf("pool creat finish\n");

  int n_events, i;
  while (1) {
    //阻塞等待事件
    n_events = epoll_wait(g_efd, evts, MAX_EVENTS + 1, 1000);
    if (n_events < 0) {
      perror("epoll_wait error");
      exit(1);
    }
    printf("有%d个事件消息\n", n_events);
    //处理读事件,紧接着发起写事件
    for (i = 0; i < n_events; i++) {
      struct event_s *ev = (struct event_s *)evts[i].data.ptr;

      if ((evts[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {
        // ev->call_back(ev);
        printf("client:%d,pid=%ld,来了读任务消息\n", i, pthread_self());
        threadpool_add(pool, ev->call_back, (void *)ev);
      }
      if ((evts[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {
        printf("client:%d,pid=%ld,来了写任务消息\n", i, pthread_self());
        threadpool_add(pool, ev->call_back, (void *)ev);
      }
    }
  }

  return 0;
}
