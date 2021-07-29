#include <arpa/inet.h>
#include <linux/un.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wrap.h>

#define PATH "serv.socket"
#define CLIE_ADDR "clie.socket"

int main(int argc, char const *argv[]) {
  int lfd = Socket(AF_UNIX, SOCK_STREAM, 0);
  char buf[BUFSIZ];

  struct sockaddr_un serv_addr, clie_addr;

  bzero(&clie_addr, sizeof(clie_addr));
  clie_addr.sun_family = AF_UNIX;
  strcpy(clie_addr.sun_path, CLIE_ADDR);

  int len = offsetof(struct sockaddr_un, sun_path) + strlen(clie_addr.sun_path);
  unlink(CLIE_ADDR);  // bind之前不能存在该文件,bind会创建该文件
  Bind(lfd, (struct sockaddr *)&clie_addr, len);  //参3不能时sizeof

  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  strcpy(serv_addr.sun_path, PATH);

  len = offsetof(struct sockaddr_un, sun_path) + strlen(serv_addr.sun_path);

  Connect(lfd, (struct sockaddr *)&serv_addr, len);

  while (fgets(buf, sizeof(buf), stdin) != NULL) {
    Write(lfd, buf, strlen(buf));
    len = Read(lfd, buf, sizeof(buf));
    Write(STDOUT_FILENO, buf, len);
  }

  return 0;
}
