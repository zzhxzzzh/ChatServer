#include "chatserver.hpp"
#include <functional>
#include <string>
#include "json.hpp"
#include "chatservice.hpp" //依赖service服务

using namespace std;
using namespace placeholders;
using json=nlohmann::json;


ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), // 初始化_server对象，传递事件循环、监听地址和名称
      _loop(loop)
{ // 初始化_loop指针，指向传入的事件循环对象

    // 给服务器注册用户连接的创建和断开的回调函数
    // 在新连接到来时或者连接断开时，会调用onConnection回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 给服务器注册用户读写事件的时候进行回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置服务器端的的线程数量 1个I/O线程,2个work线程
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 处理TCP连接的回调函数，用来处理用户连接的创建和断开
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 这里我们只是定义了一个空的回调函数，实际可以根据需求在这里处理连接事件（例如：用户登录、断开连接等）

    // 客户端断开连接
    if (!conn->connected())
    {   
        //写一个客户端异常关闭的方法
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 专门上报处理用户的读写事件的
void ChatServer::onMessage(const TcpConnectionPtr &conn, // 表示通过连接
                           Buffer *buffer,               // 缓冲区
                           Timestamp time)               // 接收到数据的时间信息
{
    string buf =buffer->retrieveAllAsString();

    //数据的反序列化
    json js=json::parse(buf);

    //达到的目的：完全的解耦网络模块的代码和业务模块的代码
    //通过js["msgid"]获取一个业务处理器，事先绑定的方法
    ///通过js["msgid"]获取=》业务handler=》conn js time

    auto msgHandler =ChatService::instance()->getHandler(js["msgid"].get<int>());
   
    /*
    getHandler返回的是一个回调函数返回的类型
    using MsgHandler=std::function<void(const TcpConnectionPtr &coon,json &js,Timestamp)>;
    所以这里要穿三个参数
    */
    //回调消息绑定好的处理器，来执行相应的业务处理，这样可以使网络模块和业务模块完全区分
    msgHandler(conn,js,time);


}
