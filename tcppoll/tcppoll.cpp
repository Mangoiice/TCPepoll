#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <string>
#include <iostream>
using namespace std;

int initserver(unsigned short port);    // 初始化监听socket的函数

int main(int argc, char * argv[])
{   
    // 输入参数检测
    if(argc != 2)
    {
        cout << "Using Example: ./tcppoll 5005" << endl;
        return -1;
    }
    // 初始化监听socket
    int listenfd = initserver(atoi(argv[1]));
    if(listenfd == -1)
    {
        perror("init");
        return -1;
    }
    /*
    创建pollfd结构体数组
    pollfd结构体有三个成员变量:fd、events(监听的事件类型)、revents
    前两个需要手动设置，每次调用poll函数，会自动修改revents的值
    */
    pollfd fds[2048];
    // 设置fd为-1
    for(int i = 0; i < 2048; i++) fds[i].fd = -1;
    // 将监听socket添加到结构体数组中
    fds[listenfd].fd = listenfd;
    fds[listenfd].events = POLLIN;
    int maxfd = listenfd;

    while(true)
    {
        int infds = poll(fds, maxfd+1, 10000);  // 超时时间单位是毫秒
        if(infds < 0)
        {
            perror("poll failed");
            break;
        }
        else if(infds == 0)
        {
            perror("poll timed out");
            continue;
        }

        for(int eventfd = 0; eventfd <= maxfd; eventfd++)
        {
            if(fds[eventfd].fd == -1) continue;
            // 判断evnetfd是否发生事件 用revents & 事件类型
            if((fds[eventfd].revents&POLLIN) == 0) continue;
            // 发生读事件的是监听socket
            if(eventfd == listenfd)
            {
                struct sockaddr_in clientaddr;
                socklen_t len = sizeof(clientaddr);
                int clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &len);

                if(clientfd < 0)
                {
                    perror("accept");
                    break;
                }
                cout << "客户端" << clientfd << "已连接" << endl;
                // 将clientfd添加到结构体数组中
                fds[clientfd].fd = clientfd;
                fds[clientfd].events = POLLIN;
                // 更新maxfd的值
                if(maxfd < clientfd) maxfd = clientfd;
            }
            // 发生读事件的是客户端socket
            else
            {
                char buffer[1024];
                int readn = recv(eventfd, buffer, 1024, 0);
                if(readn <= 0)
                {
                    cout << "客户端" << eventfd << "已断开连接" << endl;
                    close(eventfd);
                    fds[eventfd].fd = -1;
                    // 更新maxfd的值
                    if(eventfd == maxfd)
                    {
                        for(int i = maxfd; i >= 0; i--)
                        {
                            if(fds[i].fd != -1)
                            {
                                maxfd = i;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    cout << "接收到了" << buffer << endl;
                    send(eventfd, &buffer, 1024, 0);
                    cout << "发送了" << buffer << endl;
                }
            }
        }
    }

    return 0;
}

int initserver(unsigned short port)
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1)
    {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if(bind(listenfd, (const sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        perror("bind");
        return -1;
    }

    if(listen(listenfd, SOMAXCONN) != 0)
    {
        perror("listen");
        return -1;
    }

    return listenfd;
}