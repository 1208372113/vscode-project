#include <arpa/inet.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <wrap.h>

#define PATH "./serv.sock"
int main(int argc, char const *argv[]) {
  int cfd, size;
  int lfd = Socket(AF_UNIX, SOCK_STREAM, 0);
  char buf[BUFSIZ];
  unlink(PATH);
  struct sockaddr_un serv_addr, clie_addr;

  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  strcpy(serv_addr.sun_path, PATH);

  // int len = offsetof(struct sockaddr_un, sun_path) +
  // strlen(serv_addr.sun_path);
  // bind之前不能存在该文件,bind会创建该文件
  Bind(lfd, (struct sockaddr *)&serv_addr,
       sizeof(serv_addr));  //参3不能时sizeof
  Listen(lfd, 3);
  printf("accept ...\n");
  int len;
  while (1) {
    len = sizeof(clie_addr);
    cfd = Accept(lfd, (struct sockaddr *)&clie_addr, (socklen_t *)&len);

    len -= offsetof(struct sockaddr_un, sun_path);
    clie_addr.sun_path[len] = '\0';
    // printf("client bind filename %s\n", clie_addr.sun_path);

    while ((size = read(cfd, buf, sizeof(buf))) > 0) {
      for (int i = 0; i < size; i++) {
        buf[i] = toupper(buf[i]);
      }
      write(cfd, buf, size);
    }
    close(cfd);
  }

  return 0;
}
