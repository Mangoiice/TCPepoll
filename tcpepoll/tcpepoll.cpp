#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>          
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
using namespace std;

int initserver(unsigned short port);

int main(int argc, char *argv[])
{
    // 输入参数检测
    if(argc != 2)
    {
        printf("Using Example: ./tcpepoll 5005\n");
        return -1;
    }
    // 初始化监听socket
    int listenfd = initserver(atoi(argv[1]));
    if(listenfd == -1) {perror("initserver"); return -1;}
    
    // 创建一个epoll实例，类似于select的位图，本质是一个文件描述符，函数参数被忽略，但是必须大于0
    int epollfd = epoll_create(1);
    // 为监听socket准备事件结构体，事件结构体含events和一个共同体成员变量，共同体是绑定的socket
    epoll_event ev;
    ev.data.fd = listenfd;  // 绑定listenfd
    ev.events = EPOLLIN;    // 监测读事件
    // 将listenfd添加到epoll中
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
    // 定义epoll_wait返回事件的数组
    epoll_event evs[10];

    while(true)
    {
        // 调用epoll_wait，将发生的事件存储到evs中 第四个参数是超时时间
        int infds = epoll_wait(epollfd, evs, 10, -1);
        if(infds < 0)
        {
            perror("epoll_wait failed");
            break;
        }
        else if(infds == 0)
        {
            perror("epoll_wait timed out");
            continue;
        }

        for(int i = 0; i < infds; i++)
        {
            // 监听socket发生了读事件
            if(evs[i].data.fd == listenfd)
            {
                struct sockaddr_in clientaddr;
                socklen_t len;
                int clientfd;
                if((clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &len)) < 0)
                {
                    perror("accpet");
                    break;
                }
                // 将clientfd添加到epoll中
                ev.data.fd = clientfd;
                ev.events = EPOLLIN;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev);
            }
            else
            {
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                if((recv(evs[i].data.fd, buffer, sizeof(buffer), 0)) <= 0)
                {
                    printf("客户端%d断开连接\n", evs[i].data.fd);
                    close(evs[i].data.fd);
                    // epoll会自动将断开连接的socket删去
                }
                else
                {
                    printf("接收到了%s\n", buffer);
                    send(evs[i].data.fd, buffer, sizeof(buffer), 0);
                }
            }
        }
    }

    return 0;
}

int initserver(unsigned short port)
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1) {perror("socket"); return -1;}
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if((bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
    {
        perror("bind");
        return -1;
    }
    if((listen(listenfd, SOMAXCONN)) != 0)
    {
        perror("listen");
        return -1;
    }
    return listenfd;
}