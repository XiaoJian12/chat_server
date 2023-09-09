#pragma once

#include "user.h"

// User表的数据操作类
class UserModel {
public:
    bool insert(User &user);    // 插入user数据，不需要提前设置id
    User query(int id);         // 通过id查询user并返回user的所有信息
    bool updateState(User user);    // 更新对应id的user的state
    void resetState();          // 将表中所有online状态的state设置为offline
private:
};