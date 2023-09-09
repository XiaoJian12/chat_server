#pragma once

#include <string>
#include <vector>

#include "group_user.h"

class Group {
public:
    using string = std::string;

    Group(int id = -1, string name = "", string desc = "") :
        id_(id), name_(name), desc_(desc) {}
    
    void setId(int id) {id_ = id;}
    void setName(string name) {name_ = name;}
    void setDesc(string desc) {desc_ = desc;}
    
    int getId() {return id_;}
    string getName() {return name_;}
    string getDesc() {return desc_;}
    std::vector<GroupUser>& getUsers() {return users_;}
private:
    // 表字段
    int id_;
    string name_;
    string desc_;
    std::vector<GroupUser> users_;
};