#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERV_PORT 8888
#define SERV_IP "172.23.62.71"

int main(int argc, char const *argv[]) {
  int cfd = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in serv_addr;
  char buf[BUFSIZ];

  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERV_PORT);
  inet_pton(cfd, SERV_IP, &serv_addr.sin_addr);

  socklen_t len = sizeof(serv_addr);
  int n;
  while (1) {
    bzero(buf, sizeof(buf));
    fgets(buf, BUFSIZ, stdin);
    sendto(cfd, buf, strlen(buf), 0, (struct sockaddr *)&serv_addr, len);

    n = recvfrom(cfd, buf, sizeof(buf), 0, (struct sockaddr *)&serv_addr, &len);
    write(STDOUT_FILENO, buf, n);
  }
  close(cfd);

  return 0;
}
