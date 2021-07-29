#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CLIE_PORT 9000
#define MAXLINE 1500

#define GROUP "239.0.0.2"

int main(int argc, char const *argv[]) {
  struct ip_mreqn group;
  struct sockaddr_in local_addr;
  int cfd = socket(AF_INET, SOCK_DGRAM, 0);

  bzero(&local_addr, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(CLIE_PORT);
  // local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  inet_pton(AF_INET, "0.0.0.0", &local_addr.sin_addr.s_addr);

  bind(cfd, (struct sockaddr *)&local_addr, sizeof(local_addr));

  //添加到组播组
  inet_pton(AF_INET, GROUP, &group.imr_multiaddr);
  inet_pton(AF_INET, "0.0.0.0", &group.imr_address);
  group.imr_ifindex = if_nametoindex("eth0");

  setsockopt(cfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group));

  int len;
  char buf[BUFSIZ];
  while (1) {
    len = recvfrom(cfd, buf, sizeof(buf), 0, NULL, 0);
    write(STDOUT_FILENO, buf, len);
  }

  return 0;
}
