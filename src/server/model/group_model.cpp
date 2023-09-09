#include "group_model.h"
#include "db.h"

using string = std::string;

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 组装sql语句
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "insert into AllGroup(groupname, groupdesc) values('%s', '%s')",
        group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConn()));
            return true;
        }
    }

    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    // 组装sql语句
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "insert into GroupUser(groupid, userid, grouprole) values('%d', '%d', '%s')",
        groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户加入的所有群组的信息，当然也包括组员信息
std::vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "select a.id, a.groupname, a.groupdesc from AllGroup a inner join \
        GroupUser b on a.id = b.groupid where b.userid = %d",
        userid);

    // 查询userid所在的所有的组
    std::vector<Group> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                vec.push_back(group);
            }
        }
        mysql_free_result(res);
    }

    // 查询每一个组中所有的成员信息
    for (auto &group : vec)
    {
        snprintf(
            sql,
            sizeof sql,
            "select a.id, a.name, a.state, b.grouprole from User a \
            inner join GroupUser b on a.id = b.userid where groupid = %d",
            group.getId()
        );
        MySQL mysql;
        if (mysql.connect())
        {
            MYSQL_RES *res = mysql.query(sql);
            if (res != nullptr)
            {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr)
                {
                    GroupUser user;
                    user.setId(atoi(row[0]));
                    user.setName(row[1]);
                    user.setState(row[2]);
                    user.setRole(row[3]);
                    group.getUsers().push_back(user);
                }
            }
            mysql_free_result(res);
        }
    }

    return vec;
}

// 根据指定的groupid查询群成员id,userid用于排除自己，该接口用于群聊，因为不应该给自己发消息
std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid) {
    char sql[1024] = {0};
    snprintf(
        sql,
        sizeof sql,
        "select a.userid from GroupUser a where groupid = %d and userid != %d",
        groupid, userid
    );

    std::vector<int> idVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                int id = atoi(row[0]);
                idVec.push_back(id);
            }
        }
        mysql_free_result(res);
    }
    
    return idVec;
}