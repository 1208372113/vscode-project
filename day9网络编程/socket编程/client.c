#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
//#include "/usr/include/linux/in.h"

#define SERV_IP "127.0.0.1"
#define SERV_PORT 6666

int main(int argc, char const *argv[])
{
    int cfd;
    struct sockaddr_in serv_addr;
    char buf[BUFSIZ], buf2[BUFSIZ];
    int n;
    pid_t pid;

    cfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, SERV_IP, &serv_addr.sin_addr);
    connect(cfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    read(cfd, buf, BUFSIZ);
    printf("%s\n", buf);

    //设置非阻塞
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);
    pid = fork();
    if (pid == 0)
    {
        while (1)
        {
            //写
            bzero(buf, sizeof(buf));
            fgets(buf, sizeof(buf), stdin); //输入完键入回车会在字符串后添加\n,输入完成默认最后添加\0
            write(cfd, buf, sizeof(buf));
        }
    }
    else if (pid > 0)
    {
        while (1)
        {

            //读
            bzero(buf2, sizeof(buf2));
            read(cfd, buf2, sizeof(buf2));
            fputs(buf2, stdout);
        }
    }
    // while (1)
    // {
    //     读
    //     bzero(buf2, sizeof(buf2));
    //     read(cfd, buf2, sizeof(buf2));
    //     fputs(buf2, stdout);

    //     写
    //     bzero(buf, sizeof(buf));
    //     fgets(buf, sizeof(buf), stdin); //输入完键入回车会在字符串后添加\n,输入完成默认最后添加\0
    //     write(cfd, buf, sizeof(buf));
    // }

    close(cfd);
    return 0;
}
