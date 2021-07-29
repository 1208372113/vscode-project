#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include "wrap.h"
//#include "/usr/include/linux/in.h"

#define SERV_IP "127.0.0.1"
#define SERV_PORT 6666

int main(int argc, char const *argv[])
{
    int lfd, nfd;
    struct sockaddr_in serv_addr, clie_addr;
    socklen_t clie_addr_len;
    char buf[BUFSIZ], clie_ip[BUFSIZ], buf2[BUFSIZ];
    int n;
    int err;
    pid_t pid;

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == lfd)
    {
        perror("socket fail");
        return -1;
    }

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, 
        sizeof(opt));

    //设置非阻塞
    int flag = fcntl(lfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(lfd, F_SETFL, flag);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
    err = bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (-1 == err)
    {
        perror("bind fail");
        return -1;
    }

    err = listen(lfd, 32);
    if (-1 == err)
    {
        perror("wrtie fail");
    }

    while (1)
    {
        clie_addr_len = sizeof(clie_addr);
        nfd = accept(lfd, (struct sockaddr *)&clie_addr, &clie_addr_len);
        if (nfd > 0)
        {
            printf("client ip=%s,client port=%d\n", inet_ntop(AF_INET, &clie_addr.sin_addr.s_addr, clie_ip, clie_addr_len), clie_addr.sin_port);
            err = write(nfd, "欢迎链接服务器!", sizeof("欢迎链接服务器!"));
            if (-1 == err)
            {
                perror("write fail");
                return -1;
            }
            //设置非阻塞
            int flag2 = fcntl(nfd, F_GETFL);
            flag2 |= O_NONBLOCK;
            fcntl(nfd, F_SETFL, flag2);

            pid = fork();
            if (pid == 0)
            {
                while (1)
                {
                    //写操作
                    bzero(buf2, sizeof(buf2));
                    fgets(buf2, sizeof(buf2), stdin);
                    n = write(nfd, buf2, sizeof(buf2));
                    if (n == -1)
                    {
                        perror("write fail");
                    }
                }
            }
            else if (pid > 0)
            {
                while (1)
                {

                    //读操作
                    bzero(buf, sizeof(buf));
                    read(nfd, buf, sizeof(buf));
                    fputs(buf, stdout);
                }
            }
        }
    }

    close(lfd);
    close(nfd);
    return 0;
}
