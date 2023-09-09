#pragma once

#include "user.h"

// 群组用户表，由User表继承而来，多了一个role信息
class GroupUser : public User {
public:
    void setRole(string role) {role_ = role;}
    string getRole() {return role_;}
private:
    string role_;
};