// UDP通信c/s模型
#include <arpa/inet.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SERV_PORT 8888
#define SERV_IP "172.23.62.71"

int main(int argc, char const *argv[]) {
  char buf[BUFSIZ] = {0};
  struct sockaddr_in clie_addr, serv_addr;
  int n;

  int cfd = socket(AF_INET, SOCK_DGRAM, 0);

  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERV_PORT);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(cfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

  socklen_t len = sizeof(clie_addr);
  while (1) {
    bzero(buf, sizeof(buf));
    n = recvfrom(cfd, buf, sizeof(buf), 0, (struct sockaddr *)&clie_addr, &len);

    sendto(cfd, buf, n, 0, (struct sockaddr *)&clie_addr, len);
  }
  close(cfd);

  return 0;
}
