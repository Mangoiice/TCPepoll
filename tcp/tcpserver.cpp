#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        cout << "Using:./tcpservice 端口号\nExample:./tcpclient 5005" << endl;
        return -1;
    }

    // 第1步：创建服务端的监听socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1)
    {
        perror("listenfd");
        return -1;
    }

    // 第2步：把服务端用于通信的IP和端口绑定到socket上
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);   //服务端任意网卡的IP地址都可以用来通讯
    servaddr.sin_port = htons(atoi(argv[1]));
    /*
    bind函数用于将监听socket listenfd绑定到服务端的IP和端口上
    失败则返回-1 成功返回0
    */
    if(bind(listenfd, (const sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        perror("bind");
        close(listenfd);
        return -1;
    }

    // 第3步：把socket设置为可连接（监听）的状态
    if(listen(listenfd, SOMAXCONN) != 0)    //listen函数用于设置socket为被动监听，第二个参数用于指定请求队列的大小
    {
        perror("listen");
        close(listenfd);
        return -1;
    }

    // 第4步：受理客户端的连接请求，如果没有客户端连上来，accept()函数将阻塞等待
    /*
    accept函数返回连接的客户端的socket
    第一个参数为已经被listen函数设置为被动监听状态的服务端socket
    第二个参数和第三个参数用来装载客户端的IP地址信息，不需要可设为0
    */
    int clientfd = accept(listenfd, 0, 0);
    if(clientfd == -1)
    {
        perror("accept");
        close(listenfd);
        return -1;
    }
    cout << "客户端已连接" << endl;

    // 第5步：与客户端通信，接收客户端发过来的报文后，回复ok
    char buffer[1024];
    while(true)
    {
        int iret;
        memset(buffer, 0, sizeof(buffer));
        if((iret = recv(clientfd, buffer, sizeof(buffer), 0) <= 0))
        {
            cout << "iret = " << iret << endl;
            break;
        }
        cout << "接收到了报文信息:" << buffer << endl;

        strcpy(buffer, "OK");   //strcpy函数用于生成回应报文,将第二个参数地址的内容复制到第一个参数的地址
        if((iret = send(clientfd, buffer, sizeof(buffer), 0) <= 0))
        {
            perror("send");
            break;
        }
        cout << "向客户端发送了:" << buffer << endl;
    }

    // 第6步：关闭socket，释放资源
    close(listenfd);
    close(clientfd);
    return 0;
}