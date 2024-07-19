/*
最简单的一个版本，实现网络通讯的客户端程序
2024/7/19
这是我的第一个c++项目，加油吧！
*/
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

// main函数可以接收参数，argc表示接收参数的数量，是运行程序时的输入参数+1，接收到的参数存储在argv中
int main(int argc, char *argv[])
{
    // 输入参数检测，不符合会直接结束进程
    if(argc != 3)
    {
        cout << "Using:./tcpclient 服务器IP 通讯端口号\nExample:./tcpclient 192.168.207.129 5005" << endl;
        return -1;
    }

    // 第1步：创建客户端的socket

    /* 
    socket函数接收三个参数，返回socket的文件描述符
    第一个参数用来指定协议族，第二个参数用来选择是否为可靠链接，第三个参数用来选择协议族中的特定协议
    大多数情况下，第一个参数指定IPV4协议，第二个参数选择可靠链接
    第三个参数由于IPV4提供可靠链接的协议只有TCP，所以可以写0，编译器会自动选择
    socket函数的返回值如果为-1代表创建socket失败
    */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if(sockfd == -1)
    {
        perror("create socket");
        return -1;
    }

    // 第2步：向服务器发起连接请求

    /*
    首先定义一个hostent结构体指针
    因为由main函数输入的ip地址是const char *字符串，并且很有可能是一个域名而不是ip地址
    所以需要将其转化成网络字节序的数组，用于给sockaddr_in结构体的属性赋值
    而gethostbyname函数就可以做到，接收一个字符串，返回一个hostent结构体指针
    hostent结构体中的h_addr_list成员就是一个存储了多个ip地址的数组
    hostent定义了h_addr = addr_list[0]，即h_addr在大多数情况下是我们需要的ip地址，是字符串形式 "192.168.1.1"
    */
    struct hostent *tmp;
    if((tmp = gethostbyname(argv[1])) == NULL)
    {
        perror("hostent");
        close(sockfd);
        return -1;
    }
    /*
    接下来初始化储存了服务端信息的sockaddr_in结构体
    其中sin_family是协议族，和socket函数第一个参数的意义一样，直接赋值即可
    sin_port存储端口号，要求网络字节序，赋值时需要经过转换
    sin_addr是一个结构体指针，指向的结构体存放32位整数ip地址，因此有两种赋值方法:
    第一种：访问sin_addr的成员s_addr，直接对s_addr赋值，s_addr是一个32位无符号整数
    第二种，使用memcpy函数对sin_addr结构体赋值
    */
    struct sockaddr_in servaddr;    // 用于存放服务段IP和端口的结构体
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    //servaddr.sin_addr.s_addr = inet_addr(tmp->h_addr);  //inet_addr函数将ip地址字符串"192.168.1.1"转化为网络字节序的整数
    memcpy(&servaddr.sin_addr, tmp->h_addr, tmp->h_length);
    servaddr.sin_port = htons(atoi(argv[2]));   // atoi函数将数字字符串转化为数字 "12345" -> 12345

    // 调用connect函数，成功相当于服务端把连接请求加入等待队列中，不等于accept
    if((connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0))
    {
        perror("connect");
        close(sockfd);
        return -1;
    }

    // 第3步：与服务端通讯，客户发送一个请求报文后等待服务端的回复，收到回复后，再发下一个请求报文
    char buffer[1024];  // 储存发送/接收信息
    for(int i = 0; i < 3; i++)  // 进行三次通信
    {
        int iret;
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "这是第%d个报文", i);   // sprintf函数将字符串输出到指定数组中
        // 发送请求报文。报文中携带要发送的信息
        if(send(sockfd, buffer, sizeof(buffer), 0) <= 0)
        {
            perror("send"); // 这里不关闭sockfd的原因是，可能只是某一次发送出错，不影响后续发送
            break;
        }
        cout << "成功发送" << buffer << endl;

        // 接收服务端发送的回应报文,如果没有回应,recv会阻塞等待
        memset(buffer, 0, sizeof(buffer));
        if((iret = recv(sockfd, buffer, sizeof(buffer), 0)) <= 0)   // recv函数返回接收到的报文字节大小
        {
            cout << "iret = " << iret << endl;
        }
        cout << "接收到了服务端的回应报文" << buffer << endl;
    }
    // 第4步：关闭socket，释放资源
    close(sockfd);

    return 0;
}
