#pragma once

#include <vector>
#include <string>

#include "user.h"

// 好友信息表操作类
class FriendModel {
public:
    using string = std::string;

    void insert(int userid, int friendid);  // 添加好友关系

    std::vector<User> query(int userid);    // 查询userid拥有的好友
private:
};