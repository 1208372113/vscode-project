#include <stdio.h>
#include <stdlib.h>
 #include <string.h>
#include <arpa/inet.h>

int main(int argc, char const *argv[])
{
    unsigned char buf[sizeof(struct in6_addr)] ;
    int domain, s;
    char str[INET6_ADDRSTRLEN];

    if (argc != 3)
    {
        return -1;
    }

    domain = (strcmp(argv[1], "i4") == 0)   ? AF_INET
             : (strcmp(argv[1], "i6") == 0) ? AF_INET6
                                            : atoi(argv[1]);
    s = inet_pton(domain, argv[2], buf);
    for (int i = 0; i < 4; i++)
    {
        printf("%u\n", buf[i]);
    }
    
    
    if (inet_ntop(domain,buf,str,INET6_ADDRSTRLEN)==NULL)
    {
        /* code */
    }
    printf("%s\n",str);

    return 0;
}
