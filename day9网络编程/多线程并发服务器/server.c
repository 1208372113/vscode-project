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
#include <pthread.h>
#include "./include/wrap.h"

#define SERV_PORT 8888
#define SERV_IP "127.0.0.1"
#define MAXLINE 8192

struct s_info //定义一个结构体,将地址结构跟cfd捆绑
{
    struct sockaddr_in clie_addr;
    int connfd;
};

void* do_work(void* arg)
{
    int n,i;
    struct s_info *ts=(struct s_info*)arg;
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN]; //宏定义 16

    while(1)
    {
        n=Read(ts->connfd,buf,MAXLINE);
        if (n==0)
        {
            printf("the client %d closed...\n",ts->connfd);
            break;
        }
        printf("received from %s at PORT %d\n",inet_ntop(AF_INET
            ,&(ts->clie_addr.sin_addr.s_addr),str,sizeof(str))
            ,ntohs(ts->clie_addr.sin_port));
        for (i = 0; i < n; i++)
        {
            buf[i]=toupper(buf[i]);
        }
        Write(STDOUT_FILENO,buf,n);
        Write(ts->connfd,buf,n);

    }
    Close(ts->connfd);
    return (void*)0;
}
int main(int argc, char const *argv[])
{
    int lfd, cfd;
    pthread_t tid;
    struct s_info ts[256]={0};
    int n;
    int i=0;

    struct sockaddr_in serv_addr, clie_addr;//服务端客服端ip+port号
    socklen_t clie_addr_len;                //sock长度
    char buf[BUFSIZ] = {0}, clie_IP[BUFSIZ] = {0};  //缓冲区

    //创建socket
    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    //初始化服务端ip+port
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    //serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET, SERV_IP, &serv_addr.sin_addr.s_addr);

    //bind绑定
    Bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    //监听连接请求
    Listen(lfd, 128);
    printf("已准备就绪,等待客户端连接...\n");

    while (1)
    {
        //设置传出传入参数(长度)
        clie_addr_len=sizeof(clie_addr);
        cfd=Accept(lfd,(struct sockaddr*)&clie_addr,&clie_addr_len);

        //创建线程参数设置port+ip+文件描述符
        ts[i].connfd=cfd;
        ts[i].clie_addr=clie_addr;
        
        //创建线程并设置分离状态(防止称为僵尸线程)
        pthread_create(&tid,NULL,do_work,(void*)&ts[i]);
        pthread_detach(tid);
        i++;
    }
    return 0;
}
