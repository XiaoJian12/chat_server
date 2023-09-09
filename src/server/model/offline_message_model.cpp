#include "offline_message_model.h"
#include "db.h"

using string = std::string;

// 存储用户离线消息
void OfflineMsgModel::insert(int userid, string msg) {
    // 组装sql语句
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "insert into OfflineMessage values('%d', '%s')",
        userid, msg.c_str()
    );
    
    MySQL mysql;
    if (mysql.connect()) {
        mysql.update(sql);
    }
}

// 删除用户的离线消息
void OfflineMsgModel::remove(int userid) {
    // 组装sql语句
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "delete from OfflineMessage where userid = %d",
        userid
    );
    
    MySQL mysql;
    if (mysql.connect()) {
        mysql.update(sql);
    }
}

// 查询用户的离线消息
std::vector<string> OfflineMsgModel::query(int userid) {
    // 组装sql语句
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "select message from OfflineMessage where userid = %d",
        userid
    );
    
    MySQL mysql;
    std::vector<string> vec;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            // 把userid的所有离线消息返回
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                vec.push_back(row[0]);
            }
        }
        mysql_free_result(res);
    }
    return vec;
}