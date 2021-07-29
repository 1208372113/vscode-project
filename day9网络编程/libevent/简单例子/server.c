#include <arpa/inet.h>
#include <event2/event.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERV_PORT 8000
#define EVENT_LEN 20
struct event* event_s[EVENT_LEN + 1];
struct event_base* base;

void readcb(evutil_socket_t fd, short events, void* arg) {
  struct event** ev = (struct event**)arg;

  char buf[BUFSIZ] = {0};

  int n = read(fd, buf, sizeof(buf));
  if (n <= 0) {
    //从地基上删除该事件

    event_del(*ev);
    event_free(*ev);
    close(fd);
    *ev = NULL;
  } else {
    write(fd, buf, n);
  }
}

void conncb(evutil_socket_t fd, short events, void* arg) {
  int i;
  // struct event* ev = (struct event*)arg;

  //接收新的客户端连接
  int cfd = accept(fd, NULL, NULL);
  if (cfd > 0) {
    for (i = 0; i < EVENT_LEN; i++) {
      if (event_s[i] == NULL) {
        //创建一个新的事件节点
        event_s[i] =
            event_new(base, cfd, EV_READ | EV_PERSIST, readcb, &event_s[i]);
        if (event_s[i] == NULL) {
          perror("event_new error");
          event_base_loopexit(base, NULL);
        }

        //将通信事件上树
        event_add(event_s[i], NULL);

        break;
      }
    }
    if (i == EVENT_LEN) {
      printf("树已满\n");
      return;
    }
  }
}
int main(int argc, char const* argv[]) {
  struct sockaddr_in serv;
  // 1创建socket
  int lfd = socket(AF_INET, SOCK_STREAM, 0);

  // 2设置端口复用并绑定
  int opt = 1;
  setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  bzero(&serv, sizeof(serv));
  serv.sin_family = AF_INET;
  serv.sin_port = htons(SERV_PORT);
  inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr.s_addr);
  bind(lfd, (struct sockaddr*)&serv, sizeof(serv));

  // 3设置监听
  listen(lfd, 10);

  // 4创建地基
  base = event_base_new();
  if (base == NULL) {
    perror("base_new error");
  }

  // 5创建lfd对应的事件节点
  event_s[EVENT_LEN] =
      event_new(base, lfd, EV_READ | EV_PERSIST, conncb, &event_s[EVENT_LEN]);
  if (event_s[EVENT_LEN] == NULL) {
    perror("event_new error");
  }

  // 6上地基
  event_add(event_s[EVENT_LEN], NULL);

  // 7进入循环
  event_base_dispatch(base);

  // 8退出
  event_base_free(base);
  event_free(event_s[EVENT_LEN]);
  return 0;
}
