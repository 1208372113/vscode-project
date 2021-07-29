#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define LOCAL_PORT 9000
#define SERV_PORT 8888

int main(int argc, char const *argv[]) {
  int cfd = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in local_addr;
  char buf[BUFSIZ] = {0};

  bzero(&local_addr, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(LOCAL_PORT);
  inet_pton(AF_INET, "0.0.0.0",
            &local_addr.sin_addr.s_addr);  //可替代为下面这种
  // local_addr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY实际值为0

  //广播端口+ip需绑定
  int ret = bind(cfd, (struct sockaddr *)&local_addr, sizeof(local_addr));
  if (ret == 0) {
    printf("..bind..ok..\n");
  } else if (-1 == ret) {
    printf("bind fail\n");
  }

  int n;
  while (1) {
    n = recvfrom(cfd, buf, sizeof(buf), 0, NULL, 0);
    write(STDOUT_FILENO, buf, n);
  }
  close(cfd);

  return 0;
}