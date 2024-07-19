# TCP网络通讯

## 2024/7/19

编写完了tcpclient.cpp和tcpservice.cpp文件，运行无问题
最基础的网络通讯程序写完了

### 实现思路

客户端：

1. socket函数创建socket
2. connect函数连接服务端
   1. 先创建sockaddr_in结构体存储服务端信息
3. send函数发送请求报文
4. close关闭socket

服务端：

1. socket函数创建socket
2. bind函数将服务端信息绑定到socket上
3. listen函数设置监听socket
4. accept函数接受监听socket请求队列中的请求，返回客户端socket
5. recv函数和send函数用于接收/发送报文
6. close函数关闭socket

### 遇到的一些问题

某些函数的返回值记错了，导致触发了错误检测，结束了程序

## 2024/7/20

实现了对于19号编写的客户端/服务端程序的封装，将主要的实现封装成了类
同时将service统一改成了server，英语还是太差了

### 遇到的一些问题

语言基础还是不行，很多函数需要传参的类型搞不懂，哪些地方加const，哪些地方传引用搞不清
我需要再多看看书，看看视频，这个操作打算放在写完epoll模型之后