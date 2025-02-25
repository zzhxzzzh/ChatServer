#include "chatserver.hpp"
#include <iostream>
#include <chatservice.hpp>
#include <signal.h> //信号

using namespace std;

//处理服务器ctrl+c结束后，重置user的状态信息
void resethandler(int){
    ChatService::instance()->reset();//调用单例中的reset()函数。
    exit(0);
}

// int main(){
//     signal(SIGINT,resethandler);

//     EventLoop loop;
//     InetAddress addr("127.0.0.1",6000);

//     ChatServer server(&loop,addr,"ChatServer");

//     server.start();
//     loop.loop();////相当于调用epoll_wait,以阻塞方式等待新用户连接，已连接用户的读写事件等

// }

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT, resethandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}