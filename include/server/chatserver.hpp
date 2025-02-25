#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
using namespace muduo;
using namespace muduo::net;
using namespace std;



class ChatServer {
    // 定义ChatServer类，用于实现一个聊天服务器

    public:
        //初始化
        // ChatServer类的构造函数
        // 参数：
        // - loop: EventLoop指针，事件循环对象
        // - listenAddr: InetAddress对象，服务器监听地址（IP和端口）
        // - nameArg: string类型的名称，用于给服务器命名
        ChatServer(EventLoop* loop,
                   const InetAddress& listenAddr,
                   const string& nameArg);


        //开启事件循环
        void start();

    private:
        // 处理TCP连接的回调函数，用来处理用户连接的创建和断开
        void onConnection(const TcpConnectionPtr& conn);

        //专门处理用户的读写事件的
        void onMessage(const TcpConnectionPtr& conn, //表示通过连接
            Buffer* buffer, //缓冲区
            Timestamp time); //接收到数据的时间信息 


        TcpServer _server;  // 1. 定义一个TcpServer对象，用于处理TCP连接和管理事件 组合的muduo库，实现服务器功能的类对象
        EventLoop* _loop;   // 2. 创建一个EventLoop事件循环指针，用于调度网络事件 
};




#endif