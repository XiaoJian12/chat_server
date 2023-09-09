#pragma once

#include <mysql/mysql.h>
#include <string>

// 数据库操作类
class MySQL
{
public:
    using string = std::string;
    // 初始化数据库连接
    MySQL();
    // 释放数据库连接资源
    ~MySQL();
    // 连接数据库
    bool connect();
    // 更新操作
    bool update(string sql);
    // 查询操作
    MYSQL_RES *query(string sql);
    // 获取连接
    MYSQL* getConn();

private:
    MYSQL *_conn;
};