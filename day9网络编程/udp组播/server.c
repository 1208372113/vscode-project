#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERV_PORT 8000
#define CLIE_PORT 9000
#define MAXLINE 1500

#define GROUP "239.0.0.2"
// struct ip_mreqn {
//   struct in_addr imr_multiaddr; /* IP multicast address of group */
//   struct in_addr imr_address;   /* local IP address of interface */
//   int imr_ifindex;              /* Interface index */
// };

int main(int argc, char const *argv[]) {
  struct sockaddr_in serv_addr, clie_addr;
  struct ip_mreqn group;

  int cfd = socket(AF_INET, SOCK_DGRAM, 0);

  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERV_PORT);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  bind(cfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

  //发送组编号
  inet_pton(AF_INET, GROUP, &group.imr_multiaddr);
  inet_pton(AF_INET, "0.0.0.0", &group.imr_address);
  group.imr_ifindex = if_nametoindex("eth0");

  setsockopt(cfd, IPPROTO_IP, IP_MULTICAST_IF, &group, sizeof(group));

  bzero(&clie_addr, sizeof(clie_addr));
  clie_addr.sin_family = AF_INET;
  inet_pton(AF_INET, GROUP, &clie_addr.sin_addr.s_addr);
  clie_addr.sin_port = htons(CLIE_PORT);

  char buf[BUFSIZ];
  int i = 0;
  while (1) {
    bzero(buf, sizeof(buf));
    // sprintf(buf, "itcast %d\n", i++);
    fgets(buf, sizeof(buf), stdin);
    sendto(cfd, buf, sizeof(buf), 0, (struct sockaddr *)&clie_addr,
           sizeof(clie_addr));
    sleep(1);
  }

  return 0;
}
