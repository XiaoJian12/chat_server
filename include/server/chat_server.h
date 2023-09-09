#pragma once

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer {
public:
    ChatServer(
        EventLoop *loop,
        InetAddress &addr,
        const std::string &nameArg
    );

    void start();
private:
    void onConnection(const TcpConnectionPtr&);

    void onMessage(const TcpConnectionPtr&, Buffer*, Timestamp);

    TcpServer server_;
    EventLoop *loop_;
};