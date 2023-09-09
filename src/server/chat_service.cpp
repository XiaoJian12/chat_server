#include "chat_service.h"
#include "public.h"

#include <string>
#include <vector>
#include <muduo/base/Logging.h>
using namespace muduo;

ChatService::ChatService()
{
    // onMessagee将会根据msgid来从map中找到对应的handler
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    msgHandlerMap_.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    msgHandlerMap_.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    msgHandlerMap_.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    msgHandlerMap_.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});
    msgHandlerMap_.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    if (redis_.connect()) {
        redis_.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
}

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = userModel_.query(id);
    if (user.getId() == id && user.getPwd() == pwd) {
        if (user.getState() == "online") {  // 用户已登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = LOGIN_TWICE_ERR;
            response["errmsg"] = "the user has logined, you can not login twice!";
            conn->send(response.dump());
        } else {    // 登录成功
            // 更新用户登录状态
            user.setState("online");
            userModel_.updateState(user);

            {
                std::lock_guard<std::mutex> lock(connMutex_);
                // 记录用户连接
                userConnMap_.insert({user.getId(), conn});
            }

            // 登录成功就向redis订阅该用户的消息
            redis_.subscribe(id);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = SUCCESS;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询该用户是否有离线消息并返回
            std::vector<string> vec = offlineMsgModel_.query(id);
            if (!vec.empty()) {
                response["offlinemsg"] = vec;
                offlineMsgModel_.remove(id);    // 获取后删除
            }

            // 查询用户好友信息并返回
            std::vector<User> userVec = friendModel_.query(id);
            if (!userVec.empty()) {
                std::vector<string> vec2;
                for (auto &user : userVec) {
                    json res;
                    res["id"] = user.getId();
                    res["name"] = user.getName();
                    res["state"] = user.getState();
                    vec2.push_back(res.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户群组信息并返回
            std::vector<Group> groupVec = groupModel_.queryGroups(id);
            if (!groupVec.empty()) {
                // groups:[[g]]
                std::vector<string> groupV;
                for (auto &group : groupVec) {
                    json grpjson;
                    grpjson["groupid"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    std::vector<string> userV;
                    for (auto &user : group.getUsers()) {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            
            conn->send(response.dump());
        }
    } else {
        // 登录失败，用户名或密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = NAME_OR_PWD_ERR;
        response["errmsg"] = "user id or password error!";
        conn->send(response.dump());
    }
}
// 注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = userModel_.insert(user);
    if (state) {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = SUCCESS;
        response["id"] = user.getId();
        conn->send(response.dump());
    } else {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = LOGIN_TWICE_ERR;
        conn->send(response.dump());
    }
}

// 获取对应消息id的处理函数
MsgHandler ChatService::getHandler(int msgid) {
    auto it = msgHandlerMap_.find(msgid);
    if (it == msgHandlerMap_.end()) {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgid: " << msgid << "can not find handler!";
        };
    } else {
        return msgHandlerMap_[msgid];
    }
}

// 客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr& conn) {
    User user;
    {
        std::lock_guard<std::mutex> lock(connMutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); ++it) {
            if (it->second == conn) {
             // 从map表中删除用户的连接信息
                user.setId(it->first);
                userConnMap_.erase(it);
                break;
            }
        }
    }

    // 取消订阅该用户的消息
    redis_.unsubscribe(user.getId());

    // 更新用户状态
    if (user.getId() != -1) {
        user.setState("offline");
        userModel_.updateState(user);
    }
}

// 登出
void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();

    {
        std::lock_guard<std::mutex> lock(connMutex_);
        auto it = userConnMap_.find(userid);
        if (it != userConnMap_.end()) {
            userConnMap_.erase(it);
        }
    }

    // 用户登出相当于下线
    redis_.unsubscribe(userid);

    // 更新用户状态
    User user;
    user.setId(userid);
    user.setState("offline");
    userModel_.updateState(user);
}

// 服务器异常，业务重置
void ChatService::reset() {
    // 将User表中online状态的用户，设置为offline
    userModel_.resetState();
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int toid = js["friendid"].get<int>();

    {
        std::lock_guard<std::mutex> lock(connMutex_);
        auto it = userConnMap_.find(toid);
        if (it != userConnMap_.end()) { // toid在线，转发消息
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线
    User user = userModel_.query(toid);
    if (user.getState() == "online") {
        redis_.publish(toid, js.dump());
        return;
    }

    // toid不在线，存储离线消息
    offlineMsgModel_.insert(toid, js.dump());
}

// 添加好友
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    friendModel_.insert(userid, friendid);
}

// 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];

    Group group(-1, groupname, groupdesc);
    bool state = groupModel_.createGroup(group);

    if (state) {    // 创建成功
        json response;
        response["errno"] = SUCCESS;
        response["groupid"] = group.getId();
        groupModel_.addGroup(userid, group.getId(), "creator");
        conn->send(response.dump());
    } else {
        json response;
        response["errno"] = CREATE_TWICE_ERR;
        conn->send(response.dump());
    }
}

// 加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    groupModel_.addGroup(userid, groupid, "normal");
}

// 群聊
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"];
    std::vector<int> users = groupModel_.queryGroupUsers(userid, groupid);

    std::lock_guard<std::mutex> lock(connMutex_);   // 这里的锁在业务上看有问题啊，如果正在一个个发送的时候有人断开了连接，那他还会收到消息吗
    for (auto toid : users) {
        auto it = userConnMap_.find(toid);
        if (it != userConnMap_.end()) { // toid在线，转发消息
            it->second->send(js.dump());
            return;
        }

        // 如果本服务器没查到，就查询该用户是否在线
        User user = userModel_.query(toid);
        if (user.getState() == "online") {
            redis_.publish(toid, js.dump());
            return;
        }
        // toid不在线，存储离线消息
        offlineMsgModel_.insert(toid, js.dump());
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg) {
    std::lock_guard<std::mutex> lock(connMutex_);
    auto it = userConnMap_.find(userid);
    if (it != userConnMap_.end()) {
        it->second->send(msg);
        return;
    }

    // 存储离线消息
    offlineMsgModel_.insert(userid, msg);
}