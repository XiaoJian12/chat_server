#pragma once

/*
server和client的公共文件
*/

enum EnMsgType{
    LOGIN_MSG = 1,  // 登录
    LOGIN_MSG_ACK,  // 登录响应
    REG_MSG,         // 注册
    REG_MSG_ACK,     // 注册响应
    ONE_CHAT_MSG,    // 一对一聊天
    ADD_FRIEND_MSG,   // 添加好友
    LOGOUT_MSG,       // 登出

    CREATE_GROUP_MSG,   // 创建群组
    ADD_GROUP_MSG,      // 加入群组
    GROUP_CHAT_MSG      // 群聊
};

enum ErrType {
    SUCCESS,        // 成功
    NAME_OR_PWD_ERR,   // 用户名或密码错误
    LOGIN_TWICE_ERR,   // 用户已登录，不允许再登录,或者用户已存在
    CREATE_TWICE_ERR   // 群组已经存在，不能重复创建，主要是groupname有约束条件unique       
};