#include "json.hpp"

#include "group.h"
#include "user.h"
#include "public.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <ctime>
#include <numeric>

using json = nlohmann::json;
using string = std::string;

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
std::vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
std::vector<Group> g_currentUserGroupList;

// 聊天页面即menu页面控制
bool g_isLogin = false;

// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 接收线程，读取服务器发送数据
void readTaskHandler(int clientfd);
// 获取系统时间
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端，主线程为发送线程，子线程为接收线程，共两个线程
int main(int argc, char **argv) {
    if (argc != 3) { // 需要用户输入服务器ip和端口，所以参数应该有两个，argc为3
        std::cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << std::endl;
        exit(-1);
    }

    // 解析命令行参数获取ip和port
    string ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建sockfd
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd) {
        std::cerr << "create sockfd error" << std::endl;
        exit(-1);
    }

    sockaddr_in server;
    ::bzero(&server, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip.c_str());

    if (-1 == ::connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in))) {
        std::cerr << "connect error" << std::endl;
        close(clientfd);
        exit(-1);
    }

    // 将用户从命令行写入的数据发送出去
    for (;;) {
        // 显示首页菜单、登录、注册、退出
        std::cout << "=======================" << std::endl;
        std::cout << "1. login" << std::endl;
        std::cout << "2. register" << std::endl;
        std::cout << "3. quit" << std::endl;
        std::cout << "=======================" << std::endl;
        std::cout << "choice:";
        int choice = 0;
        //std::cin.clear();       // 以防有错误输入
        std::cin >> choice;
        std::cin.get();
        //std::cin.ignore(std::numeric_limits<std::streamsize>::max());     // 以防有错误输入

        switch (choice)
        {
        case 1 : // login
            {
                int id = 0;
                char pwd[50] = {0};
                std::cout << "id:";
                std::cin >> id;
                std::getchar();     // 读取掉缓冲区残余的回车
                std::cout << "password:";
                std::cin.getline(pwd, 50);  // 遇到回车就清除回车并返回

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                int len = ::send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1) {
                    std::cerr << "send login msg error, id:" << id << std::endl;
                } else {
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);
                    if (-1 == len) {
                        std::cerr << "recv reg response error" << std::endl;
                    } else {
                        json response = json::parse(buffer);
                        if (SUCCESS != response["errno"].get<int>()) {    // 登录失败
                            std::cerr << response["errmsg"]  << std::endl;
                            break;
                        } else {    // 登录成功
                            g_isLogin = true;

                            // 记录从服务器返回的当前用户信息
                            g_currentUserFriendList.clear();
                            g_currentUser.setId(response["id"].get<int>());
                            g_currentUser.setName(response["name"]);

                            // 记录当前用户好友信息
                            if (response.contains("friends")) {
                                std::vector<string> vec = response["friends"];
                                for (auto &str : vec) {
                                    json temp = json::parse(str);
                                    User user;
                                    user.setId(temp["id"].get<int>());
                                    user.setName(temp["name"]);
                                    user.setState(temp["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }

                            // 记录当前用户群组信息
                            if (response.contains("groups")) {
                                g_currentUserGroupList.clear();
                                std::vector<string> vec1 = response["groups"];
                                for (auto &groupstr : vec1) {
                                    json grpjs = json::parse(groupstr);
                                    Group group;
                                    group.setId(grpjs["groupid"].get<int>());
                                    group.setName(grpjs["groupname"]);
                                    group.setDesc(grpjs["groupdesc"]);
                                    std::vector<string> users = grpjs["users"];
                                    for (string &userstr : users) {
                                        GroupUser user;
                                        json js = json::parse(userstr);
                                        user.setId(js["id"].get<int>());
                                        user.setName(js["name"]);
                                        user.setState(js["state"]);
                                        user.setRole(js["role"]);
                                        group.getUsers().push_back(user);
                                    }
                                    g_currentUserGroupList.push_back(group);
                                }
                            }

                            // 显示当前登录用户的基本信息
                            showCurrentUserData();

                            // 显示离线消息
                            if (response.contains("offlinemsg")) {
                                std::vector<string> vec = response["offlinemsg"];
                                for (string &str : vec) {
                                    json js = json::parse(str);

                                    std::cout << js["time"] << "[" << js["id"] <<
                                        "]" << js["name"] << " said: " << js["msg"] << std::endl;
                                }
                            }

                            // 登录成功，启动读线程负责接收服务器发送过来的数据
                            static int threadNum = 0;
                            if (threadNum++ == 0) {
                                std::thread readTask(readTaskHandler, clientfd);
                                readTask.detach();
                            }

                            // 进入聊天主界面菜单
                            //g_isLogin = true;
                            mainMenu(clientfd);
                        }
                    }
                }
            }
            break;
        case 2 : // register
            {
                char name[50] = {0};
                char pwd[50] = {0};
                std::cout << "username:";
                std::cin.getline(name, 50);
                std::cout << "userpassword:";
                std::cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();

                int len = ::send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1) {
                    std::cerr << "send reg msg error:" << request << std::endl;
                } else {
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);
                    if (-1 == len) {
                        std::cerr << "recv reg response error" << std::endl;
                    } else {
                        json response = json::parse(buffer);
                        if (0 != response["errno"].get<int>()) {    // 注册失败
                            std::cerr << name << "is already exitst!" << std::endl;
                        } else {
                            std::cout << name << "register successfully, userid is" <<
                                response["id"] << ", do not forget it!" << std::endl;
                        }
                    }
                }
            }
            break;
        case 3 : // quit
            close(clientfd);
            exit(0);
            break;
        default:
            std::cerr << "invalid input!" << std::endl;
            break;
        }

    }
}

void showCurrentUserData() {
    std::cout << "===============login user================" << std::endl;
    std::cout << "current login user => id:" << g_currentUser.getId() << "name:" << g_currentUser.getName() << std::endl;
    std::cout << "---------------friend list-----------------" << std::endl;
    if (!g_currentUserFriendList.empty()) {
        for (User &user : g_currentUserFriendList) {
            std::cout << user.getId() << " " << user.getName() << " " << user.getState() << std::endl;
        }
    }
    std::cout << "---------------group list-----------------" << std::endl;
    if (!g_currentUserGroupList.empty()) {
        for (auto & group : g_currentUserGroupList) {
            std::cout << group.getId() << " " << group.getName() << " " << group.getDesc() << std::endl;
            for (auto &user : group.getUsers()) {
                std::cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << std::endl;
            }
        }
    }
    std::cout << "==========================================" << std::endl;
}

// 接收线程，读取服务器发送数据
void readTaskHandler(int clientfd) {
    for (;;) {
        char buffer[1024] = {0};
        int len = ::recv(clientfd, buffer, sizeof buffer, 0);
        if (-1 == len || 0 == len) {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        if (ONE_CHAT_MSG == js["msgid"].get<int>()) {
            std::cout << js["time"] << " user" << "[" << js["id"] <<
                "]" << js["name"] << " said: " << js["msg"] << std::endl;
        } else if (GROUP_CHAT_MSG == js["msgid"].get<int>()) {
            int groupid = js["groupid"].get<int>();
            std::cout << js["time"] << " from group " << groupid << " by user" << "[" << js["id"] <<
                "]" << js["name"] << " said: " << js["msg"] << std::endl;
        }
    }
}

// 获取系统时间
string getCurrentTime() {
    std::time_t currentTime = std::time(nullptr);
    char buffer[80] = {0};
    std::strftime(buffer, sizeof buffer, "%Y/%m/%d %H:%M:%S", std::localtime(&currentTime));

    return buffer;
}
// "help" command handler"
void help(int fd = 0 ,string str = "");

// "chat" command handler"
void chat(int, string);

// "addfriend" command handler"
void addfriend(int, string);

// "creategroup" command handler"
void creategroup(int, string);

// "addgroup" command handler"
void addgroup(int, string);

// "groupchat" command handler"
void groupchat(int, string);

// "logout" command handler"
void logout(int, string);

// 系统支持的客户端命令列表
std::unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令, 格式help"},
    {"chat", "一对一聊天, 格式chat:friendid:message"},
    {"addfriend", "添加好友, 格式addfriend:friendid"},
    {"creategroup", "创建群组, 格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组, 格式addgroup:groupid"},
    {"groupchat", "群聊, 格式groupchat:groupid:message"},
    {"logout", "登出, 格式logout"}
};

// 函数对象表
std::unordered_map<string, std::function<void(int, string)>> commanHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout}
};

// 主聊天页面程序
void mainMenu(int clientfd) {
    help();

    char buffer[1024] = {0};
    while (g_isLogin) {
        std::cin.getline(buffer, sizeof buffer);
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(':');
        if (-1 == idx) {
            command = commandbuf;
        } else {
            command = commandbuf.substr(0, idx);
        }
        auto it = commanHandlerMap.find(command);
        if (it == commanHandlerMap.end()) {
            std::cerr << "invalid command, please type again or type help" << std::endl;
            continue;
        }

        it->second(clientfd, commandbuf.substr(idx + 1));   // 第一个':'当然不需要了
    }
}

// "help command handler"
void help(int fd ,string str) {
    std::cout << "show command list >>> " << std::endl;
    for (auto &p : commandMap) {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}

// "chat" command handler"
void chat(int clientfd, string str) {
    json js;
    int idx = str.find(':');
    if (-1 == idx) {
        std::cerr << "invalid command!" << std::endl;
    }
    int friendid = std::stoi(str.substr(0, idx));
    string msg = str.substr(idx + 1);
    string time = getCurrentTime();
    int id = g_currentUser.getId();
    string name = g_currentUser.getName();

    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = id;
    js["name"] = name;
    js["msg"] = msg;
    js["time"] = time;
    js["friendid"] = friendid;
    string request = js.dump();

    int len = ::send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len) {
        std::cerr << "send message to id:" << friendid << " error." << std::endl;
    } else {
        std::cout << "successfully!" << std::endl;
    }
    std::cout << "You can type command now!" << std::endl;
}

// "addfriend" command handler"
void addfriend(int clientfd, string str) {
    json js;
    int friendid = std::stoi(str);
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string request = js.dump();

    int len = ::send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len) {
        std::cerr << "send addfriend error ->" << request << std::endl;
    } else {
        std::cout << "successfully!" << std::endl;
    }
    std::cout << "You can type command now!" << std::endl;
}

// "creategroup" command handler"
void creategroup(int clientfd, string str) {
    json js;
    int idx = str.find(':');
    if (-1 == idx) {
        std::cerr << "invalid command!" << std::endl;
        std::cout << "You can type command now!" << std::endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1);
    
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string request = js.dump();

    int len = ::send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len) {
        std::cerr << "send create group faild!" << std::endl;
    }

    char buffer[100] = {0};
    len = ::recv(clientfd, buffer, sizeof buffer, 0);
    if (-1 == len) {
        std::cerr << "recv creategroup error!" << std::endl;
    } else {
        json recvjs = json::parse(buffer);
        if (recvjs.contains("errno") && recvjs["errno"] == SUCCESS) {
            std::cout << "creategroup successfully!" << std::endl;
            std::cout << "Your groupid is [" << recvjs["groupid"] << "], please do not forget it!" << std::endl;
        } else {
            std::cerr << "creategroup failed, maybe you should use another groupname." << std::endl;
        }
    }
    std::cout << "You can type command now!" << std::endl;
}

// "addgroup" command handler"
void addgroup(int clientfd, string str) {
    json js;
    int groupid = std::stoi(str);

    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string request = js.dump();

    int len = ::send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len) {
        std::cerr << "addgroup failed." << std::endl;
    } else {
        std::cout << "successfully!" << std::endl;
    }
    std::cout << "You can type command now!" << std::endl;
}

// "groupchat" command handler"
void groupchat(int clientfd, string str) {
    // 格式groupchat:groupid:message
    json js;
    int idx = str.find(':');
    if (-1 == idx) {
        std::cerr << "invalid command!" << std::endl;
        std::cout << "You can type command now!" << std::endl;
        return;
    }

    int groupid = std::stoi(str.substr(0, idx));
    string msg = str.substr(idx + 1);
    string time = getCurrentTime();
    int id = g_currentUser.getId();
    string name = g_currentUser.getName();

    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = id;
    js["name"] = name;
    js["groupid"] = groupid;
    js["msg"] = msg;
    js["time"] = time;
    string request = js.dump();

    int len = ::send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len) {
        std::cerr << "send message to group " << groupid << "failed!" << std::endl;
    } else {
        std::cout << "successfully!" << std::endl;
    }
    std::cout << "You can type command now!" << std::endl;
}

// "logout" command handler"
void logout(int clientfd, string str) {
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_currentUser.getId();
    string request = js.dump();

    int len = ::send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len) {
        std::cerr << "logout error!" << std::endl;
    } else {
        g_isLogin = false;
        std::cout << "Thanks for using this client, see you next time!" << std::endl;
    }
}