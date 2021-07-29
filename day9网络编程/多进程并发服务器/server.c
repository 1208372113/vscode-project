#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <strings.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include "./include/wrap.h"

#define SERV_PORT 8888
#define SERV_IP "127.0.0.1"

void wait_child(int signo)
{
    while (waitpid(0, NULL,WNOHANG)>0)
    {
        
    }
    return;
}
int main(int argc, char const *argv[])
{
    int lfd, cfd;
    pid_t pid;
    int n;
    struct sockaddr_in serv_addr, clie_addr;
    socklen_t clie_addr_len;
    char buf[BUFSIZ]={0},clie_IP[BUFSIZ]={0};

    lfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    //serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET, SERV_IP, &serv_addr.sin_addr.s_addr);

    Bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    Listen(lfd, 128);

    while (1)
    {
        clie_addr_len = sizeof(clie_addr);
        cfd = Accept(lfd, (struct sockaddr *)&clie_addr,
                     &clie_addr_len);
        printf("client ip:%s,port:%d\n", inet_ntop(AF_INET, 
            &clie_addr.sin_addr.s_addr, clie_IP, 
            sizeof(clie_IP)),ntohs(clie_addr.sin_port));
        pid = fork();
        if (pid < 0)
        {
            perror("fork fail");
            exit(1);
        }
        else if (pid > 0) //父进程
        {
            close(cfd);
            signal(SIGCHLD, wait_child);
        }
        else
        {
            close(lfd);
            break;
        }
    }

    if (pid == 0)
    {
        close(lfd);
        Write(cfd, "欢迎连接本服务器\n", sizeof("欢迎连接本服务器\n"));
        while (1)
        {
            bzero(buf, sizeof(buf));
            n = Read(cfd, buf, sizeof(buf));
            if (n == 0) //clien close
            {
                close(cfd);
                return 0;
            }
            else if (n == -1)
            {
                perror("read error");
                exit(1);
            }
            else
            {
                for (int i = 0; i < n; i++)
                {
                    buf[i] = toupper(buf[i]);
                }
                Write(cfd, buf, sizeof(buf));
                Write(STDOUT_FILENO,buf,sizeof(buf));
            }
        }
    }
    return 0;
}
