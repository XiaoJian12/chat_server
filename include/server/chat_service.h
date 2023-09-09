#pragma once

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "user_model.h"
#include "offline_message_model.h"
#include "friend_model.h"
#include "group_model.h"
#include "json.hpp"
#include "redis.hpp"

using json = nlohmann::json;

using namespace muduo;
using namespace muduo::net;
using namespace std::placeholders;
using string = std::string;

using MsgHandler = std::function<void(const TcpConnectionPtr&, json&, Timestamp)>;

class ChatService {
public:
    // 获取单例
    static ChatService* instance();
    // 登录业务
    void login(const TcpConnectionPtr&, json&, Timestamp);
    // 注册业务
    void reg(const TcpConnectionPtr&, json&, Timestamp);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 登出
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群聊
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取对应消息id的处理函数
    MsgHandler getHandler(int msgid);

    // 客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);
    // 服务器异常，业务重置
    void reset();

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);
private:
    ChatService();

    std::unordered_map<int, MsgHandler> msgHandlerMap_; // 存储消息id及其处理函数

    std::unordered_map<int, TcpConnectionPtr> userConnMap_; // 存储登录的连接

    std::mutex connMutex_;  // 对userConnMap_加锁操作

    UserModel userModel_;   // User表操作对象
    OfflineMsgModel offlineMsgModel_;   // OfflineMessage表操作对象
    FriendModel friendModel_;   // Friend表操作对象
    GroupModel groupModel_;     // AllGroup表和GroupUser表操作对象

    Redis redis_;   // redis操作对象

};