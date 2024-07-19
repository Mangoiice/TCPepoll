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

## 2024/7/21-2

最担心的事情还是发生了，为了使用类，导致写的select模型实际上仍然只能一个服务端对应一个客户端
这个问题在写poll模型的时候体现出来了，于是写poll模型的时候干脆不使用类了，所有代码除了初始化监听socket都在main函数中了，等下次再修改select模型的代码吧

## 2024/7/21-3

写了epoll的水平触发模型，有一点乱，捋一下思路

1. 先创建一个epoll的句柄，这个东西就像select中的位图，poll中的pollfd数组，是用来存储监测的socket的
2. 单独设置一个epoll_event结构体，通过设置这个结构体，设置好之后调用epoll_ctl函数将其加入到epoll中
3. 然后调用epoll_wait函数监测epoll中的socket，发生了的事件会存储在提前定义好的evs结构体数组中，通过逐个访问这个数组中的元素，可以进行事件处理，也正因如此，epoll模型不需要轮询所有socket

## select、poll、epoll模型对比

相似点：

1. 三个模型都有用于存放监测socket的东西(我称呼其为socket池)。select是fd_set类型的位图，本质上是一个存放32个数的int数组；poll模型是存放pollfd结构体的数组，数组的大小可以自己定义，这也是区别于select模型的地方；epoll模型则是通过epoll_create函数创建的一个epoll实例，本质上是和socket一样的文件描述符
2. 三个模型的代码都有事件循环的部分，视觉上是一个while(true)循环。select模型在这个循环中轮询位图，通过FD_ISSET函数来判断是否发生了事件并处理；poll模型通过结构体的revents成员变量判断是否发生事件并处理；epoll模型则是通过epoll_wait函数，修改提前定义的epoll_event数组，避免了轮询
3. 不管是客户端连接，还是断开连接，三者都有自己的修改socket池的方法。select是通过FD_CLR修改原始fd_set；poll模型是将pollfd结构体的fd成员变量修改为-1，在事件循环中忽略；epoll模型如果添加socket，是先通过设置epoll_event结构体，然后通过epoll_ctl函数将其添加到池子中，客户端断开连接的话epoll会自动删除

不同点

1. select和poll模型只有水平触发，epoll模型可以设置为边缘触发
2. select和poll模型是轮询方法，效率偏低，epoll模型可以直接锁定发生事件的socket
3. 前两个模型需要记录maxfd的值，来进行查找，以降低时间复杂度
