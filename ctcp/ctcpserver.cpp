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
private:
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

int main(int argc,char *argv[])
{
  if (argc!=2)
  {
    cout << "Using:./demo8 通讯端口\nExample:./demo8 5005\n\n";   // 端口大于1024，不与其它的重复。
    cout << "注意:运行服务端程序的Linux系统的防火墙必须要开通5005端口。\n";
    cout << "      如果是云服务器，还要开通云平台的访问策略。\n\n";
    return -1;
  }

  ctcpserver tcpserver;
  if (tcpserver.initserver(atoi(argv[1]))==false) // 初始化服务端用于监听的socket。
  {
    perror("initserver()"); return -1;
  }

  // 受理客户端的连接（从已连接的客户端中取出一个客户端），  
  // 如果没有已连接的客户端，accept()函数将阻塞等待。
  if (tcpserver.accept()==false)
  {
    perror("accept()"); return -1;
  }
  cout << "客户端已连接(" << tcpserver.clientip() << ")。\n";

  string buffer;
  while (true)
  {
    // 接收对端的报文，如果对端没有发送报文，recv()函数将阻塞等待。
    if (tcpserver.recv(buffer,1024)==false)
    {
      perror("recv()"); break;
    }
    cout << "接收：" << buffer << endl;
 
    buffer="ok";  
    if (tcpserver.send(buffer)==false)  // 向对端发送报文。
    {
      perror("send"); break;
    }
    cout << "发送：" << buffer << endl;
  }
}
