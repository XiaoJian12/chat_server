#pragma once

#include "group.h"

#include <string>
#include <vector>

// Group表和GroupUser表操作类
class GroupModel {
public:
    using string = std::string;

    // 创建群组，insert into AllGroup
    bool createGroup(Group &group);
    // 加入群组，insert into GroupUser
    void addGroup(int userid, int groupid, string role);
    // 查询用户加入的所有群组的信息
    std::vector<Group> queryGroups(int userid);
    // 根据指定的groupid查询群成员id,userid用于排除自己，该接口用于群聊
    std::vector<int> queryGroupUsers(int userid, int groupid);
private:
};