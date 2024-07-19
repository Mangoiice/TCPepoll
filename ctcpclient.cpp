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

class ctcpclient
{
private:
    int m_clientfd;
    string m_ip;
    unsigned short m_port;
public:
    // 构造函数，初始化m_clientfd为-1
    ctcpclient(){m_clientfd = -1;}
    // 像服务器发起连接请求，成功返回true，失败返回false
    bool connect(const string &in_ip, const unsigned short in_port)
    {
        // 如果socket已连接，直接返回失败
        if(m_clientfd == -1) return false;
        // 把服务器的IP和端口保存在成员变量中
        m_ip = in_ip;
        m_port = in_port;
        // 创建socket
        if(( m_clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return false;

        // 连接服务器
        struct hostent *tmp;
        if((tmp = gethostbyname(m_ip.c_str())) == nullptr)  // 获取主机IP
        {
            ::close(m_clientfd);
            m_clientfd = -1;
            return false;
        }

        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(tmp->h_addr);
        servaddr.sin_port = htons(m_port);
        if((::connect(m_clientfd, (const sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        {
            ::close(m_clientfd);
            m_clientfd = -1;
            return false;
        }
        return true;
    }

    // 向服务器发送报文，成功返回true，失败返回false
    bool send(const string &buffer)
    {
        if(m_clientfd == -1) return false;
        if((::send(m_clientfd, buffer.data(), buffer.size(), 0)) <= 0) return false;
        return true;
    }

    // 从服务器接收报文，成功返回true，失败返回false
    bool recv(string &buffer, const size_t maxlen)
    {
        // 如果直接操作string对象的内存，必须保证：1)不能越界；2）操作后手动设置数据的大小
        buffer.clear();         // 清空buffer
        buffer.resize(maxlen);  // 设置buffer的大小为maxlen。
        int readn=::recv(m_clientfd,&buffer[0],buffer.size(),0);  // 直接操作buffer的内存。
        if (readn<=0) 
        {
            buffer.clear();
            return false;
        }
        buffer.resize(readn);   // 重置buffer的实际大小为接收到的报文大小
        return true;
    }

    // 断开与服务器的连接
    bool close()
    {
        if(m_clientfd == -1) return false;
        ::close(m_clientfd);
        m_clientfd = -1;
        return true;
    }

    // 析构函数，在类销毁后自动关闭socket
    ~ctcpclient(){close();}
};

int main(int argc, char *argv[])
{
    // 输入参数检测，不符合会直接结束进程
    if(argc != 3)
    {
        cout << "Using:./tcpclient 服务器IP 通讯端口号\nExample:./tcpclient 192.168.207.129 5005" << endl;
        return -1;
    }

    ctcpclient tcpclient;
    if(!tcpclient.connect("192.168.207.129", 5005))
    {
        perror("connect()");
        return -1;
    }

    string buffer;
    for(int i = 0; i < 3; i++)
    {
        buffer = "这是第" + to_string(i) + "个请求报文";    // 生成请求报文，拼接时需要其中一个为string
        if(!tcpclient.send(buffer))
        {
            perror("send()");
            break;
        }
        cout << "发送了" << buffer;
        buffer.clear();

        if (tcpclient.recv(buffer,1024)==false)
        {
            perror("recv()"); break;
        }
        cout << "接收：" << buffer << endl;

        buffer.clear();
    }
}