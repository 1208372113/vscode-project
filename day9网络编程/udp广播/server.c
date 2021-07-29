#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERV_PORT 8888

#define CLIE_PORT 9000  // 广播时客户端端口号需要确定值

#define BROADCAST_IP "172.23.63.255"  // ifconfig查看广播地址

int main(int argc, char const *argv[]) {
  int cfd;
  struct sockaddr_in clie_addr, serv_addr;
  char buf[BUFSIZ] = {0};

  cfd = socket(AF_INET, SOCK_DGRAM, 0);

  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERV_PORT);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  bind(cfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

  int flag = 1;
  setsockopt(cfd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag));

  bzero(&clie_addr, sizeof(clie_addr));
  clie_addr.sin_family = AF_INET;
  clie_addr.sin_port = htons(CLIE_PORT);
  inet_pton(AF_INET, BROADCAST_IP, &clie_addr.sin_addr.s_addr);

  int i = 0;
  while (1) {
    sprintf(buf, "Drink %d water\n", i++);
    sendto(cfd, buf, strlen(buf), 0, (struct sockaddr *)&clie_addr,
           sizeof(clie_addr));
    sleep(1);
  }

  return 0;
}
