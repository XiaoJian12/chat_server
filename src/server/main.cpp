#include "chat_server.h"
#include "chat_service.h"
#include <iostream>
#include <signal.h>

void resetHandler(int) {
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv) {
    signal(SIGINT, resetHandler);

    if (argc != 3) { // 需要用户输入服务器ip和端口，所以参数应该有两个，argc为3
        std::cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << std::endl;
        exit(-1);
    }

    // 解析命令行参数获取ip和port
    string ip = argv[1];
    uint16_t port = atoi(argv[2]);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "chat");

    server.start();
    loop.loop();

    return 0;
}