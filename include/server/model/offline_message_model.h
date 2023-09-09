#pragma once

#include <string>
#include <vector>

class OfflineMsgModel {
public:
    using string = std::string;
    // 存储用户离线消息
    void insert(int userid, string msg);

    // 删除用户的离线消息
    void remove(int userid);

    // 查询用户的离线消息
    std::vector<string> query(int userid);
private:
};