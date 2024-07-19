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

class ctcpserver
{
public:
    int m_listenfd;
    int m_clientfd;
    string m_clientip;
    unsigned short m_port;

public:
    // 构造函数，初始化监听socket和客户端socket为-1
    ctcpserver()
    {
        m_clientfd = -1;
        m_listenfd = -1;
    }
    // 初始化服务端scoket
    bool initserver(unsigned short port)
    {
        m_port = port;
        // 如果监听socket已经初始化，直接返回false
        if(m_listenfd != -1) return false;
        m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
        // 初始化监听socket，先将服务端信息绑定到监听socket上
        struct sockaddr_in servaddr;
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(m_port);

        if((bind(m_listenfd, (const sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        {
            close(m_listenfd);
            m_listenfd = -1;
            return false;
        }
        // 设置m_listenfd为监听socket
        if((listen(m_listenfd, SOMAXCONN)) != 0) 
        {
            close(m_listenfd);
            m_listenfd = -1;
            return false;
        }

        return true;
    }

    // 接受客户端的连接请求
    bool accept()
    {
        if(m_clientfd != -1) return false;
        struct sockaddr_in clientaddr;
        socklen_t addrlen=sizeof(clientaddr);   // struct sockaddr_in的大小。
        if ((m_clientfd=::accept(m_listenfd,(struct sockaddr *)&clientaddr,&addrlen)) == -1) return false;

        m_clientip=inet_ntoa(clientaddr.sin_addr);  // 把客户端的地址从大端序转换成字符串。
        return true;
    }

    // 获取客户端的IP(字符串格式)。
    const string & clientip() const
    {
        return m_clientip;
    }

    // 向对端发送报文，成功返回true，失败返回false。
    bool send(const string &buffer)   
    {
        if (m_clientfd==-1) return false;
        if ( (::send(m_clientfd,buffer.data(),buffer.size(),0))<=0) return false;
        return true;
    }

    // 接受请求报文
    bool recv(string &buffer, const size_t maxlen)
    {
        buffer.clear();
        buffer.resize(maxlen);
        int readn = ::recv(m_clientfd, &buffer[0], buffer.size(), 0);
        if(readn <= 0)
        {   
            buffer.clear();
            return false;
        }
        buffer.resize(readn);
        return true;
    }

    // 关闭监听的socket。
    bool closelisten()
    {
        if (m_listenfd==-1) return false; 

        ::close(m_listenfd);
        m_listenfd=-1;
        return true;
    }

    // 关闭客户端连上来的socket。
    bool closeclient()
    {
        if (m_clientfd==-1) return false;

        ::close(m_clientfd);
        m_clientfd=-1;
        return true;
    }

    ~ctcpserver()
    {
        closeclient();
        closelisten();
    }
};

int main(int argc, char *argv[])
{
    // 检测输入参数
    if(argc != 2)
    {
        cout << "Using Example: ./tcpselect 服务端端口" << endl;
        return -1;
    }

    // 创建服务端类
    ctcpserver tcpserver;
    // 调用类的initserver方法，创建了监听socket并且绑定到了服务端IP和端口上
    if(!(tcpserver.initserver(atoi(argv[1]))))
    {
        perror("initserver");
        return -1;
    }

    // 创建位图readfds
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(tcpserver.m_listenfd, &readfds);
    // 设置readfds的最大值
    int maxfd = tcpserver.m_listenfd;

    // 进行读事件检测
    while(true)
    {
        // 设置select超时结构体，设置10s为超时时间
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        // 拷贝readfds,因为select会修改位图
        fd_set tmpfds = readfds;
        /*
        调用select函数，会把发生事件的socket设为1
        第一个参数是readfds最大值+1
        第二个参数是监测读事件的位图
        第三个参数是监听写事件的位图，一般不关心，设为NULL
        第四个参数是监听异常时间的位图，在IO中会用到，网络通讯不关心，设为NULL
        第五个参数是指向超时结构体的指针
        返回<0代表出现异常，=0代表超时，>0表示发生的事件个数
        */
        int infds = select(maxfd+1, &tmpfds, NULL, NULL, 0);
        // 判断infds即select的返回值，只有>0才会开始处理事件
        if(infds < 0)
        {
            perror("select failed");
            break;
        }
        
        if(infds == 0)
        {
            perror("select timed out");
            continue;
        }
        // 遍历位图，找到发生事件的socket
        for(int eventfd = 0; eventfd <= maxfd; eventfd++)
        {
            if(FD_ISSET(eventfd, &tmpfds) == 0) continue;
            /*
            读事件类型：
            1、有新的socket连接
            2、缓冲区有数据可读
            3、socket断开连接
            如果发生事件的是监听socket，则只有可能是第一种读事件
            */
            if(eventfd == tcpserver.m_listenfd)
            {
                // 判断连接是否成功，不成功继续连接，因为select是水平触发
                if(!(tcpserver.accept()))
                {
                    perror("accept");
                    continue;
                }
                
                cout << "客户端" << tcpserver.clientip() << "已连接" << endl;

                // 将连接上的客户端socket添加到位图中并且更新maxfd的值
                FD_SET(tcpserver.m_clientfd, &readfds);
                if(tcpserver.m_clientfd > maxfd) maxfd = tcpserver.m_clientfd;
            }
            else
            {
                // 非监听socket发生了事件，开始处理
                string buffer;
                if(!(tcpserver.recv(buffer, 1024)))
                {
                    perror("recv");
                    // 客户端已经断开连接，需要关闭客户端socket，并设置位图信息
                    FD_CLR(tcpserver.m_clientfd, &readfds);
                    tcpserver.closeclient();
                    if(tcpserver.m_clientfd == maxfd)
                    {
                        for(int i = maxfd; i > 0; i--)
                        {
                            if(FD_ISSET(i, &readfds))
                            {
                                maxfd = i;
                                break;
                            }
                        }
                    }
                    continue;
                }
                else
                {
                    cout << "接收到了" << buffer << endl;
                    buffer = "ok";
                    if(!(tcpserver.send(buffer)))
                    {
                        perror("send");
                        continue;
                    }
                    cout << "发送了" << buffer << endl;
                }                 
            }
        }
    }
    return 0;
}