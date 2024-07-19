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

语言基础还是不行，很多函数需要传参的类型搞不懂，哪些地方加const，哪些地方传引用搞不清
我需要再多看看书，看看视频，这个操作打算放在写完epoll模型之后

## 2024/7/21-1

写了select模型的服务端，写代码的时候编译无问题
运行起来就出了一堆问题

1. 客户端连不上，原因是ctcpserver类的initserver方法中已经把端口号htons了，在传参的时候又htons了一遍
2. 客户端connect，但是服务端收不到消息，原因是编写ctcpserver中的类的时候，accept函数写错了，昨天没有测试，哎。错误原因是accept出错判断应该是！=0，写成了==0，导致::ccept成功了也会返回错误信息
3. 客户端发送的信息接收不到，原因是写select模型的时候，在遍历位图的判断是否发生事件的if语句后多了个分号，导致if语句包含的continue一直在执行，无法进行后续事件处理，这也是由于continue写在了和if语句同一行，看不到
4. 客户端发送完信息后再次调用select报错，原因是断开客户端连接，先调用了ctcpserver类的closeclient方法，把clientfd置为-1。然后才清空的位图，导致位图出错，类的通用性有点差
