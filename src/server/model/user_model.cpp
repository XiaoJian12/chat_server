#include "user_model.h"
#include "db.h"

bool UserModel::insert(User &user) {
    // 组装sql语句
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "insert into User(name, password, state) values('%s', '%s', '%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str()
    );

    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            user.setId(mysql_insert_id(mysql.getConn()));
            return true;
        }
    }

    return false;
}

User UserModel::query(int id) {
    // 组装sql语句
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "select * from User where id = %d",
        id
    );

    MySQL mysql;
    User user;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr) {
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
            }
        }
        mysql_free_result(res);
    }
    return user;
}

bool UserModel::updateState(User user) {
    // 组装sql语句
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "update User set state = '%s' where id = %d",
        user.getState().c_str(), user.getId()
    );

    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            return true;
        }
    }

    return false;
}

void UserModel::resetState() {
    // 组装sql语句
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "update User set state = 'offline' where state = 'online'"
    );

    MySQL mysql;
    if (mysql.connect()) {
        mysql.update(sql);
    }
}