//user这张表的数据操作类

#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

//user这张表的数据操作类
class UserModel{

 public:
    
    //User表的增加方法
    bool insert(User &user);

    //User表的查询方法
    //根据用户id查询用户信息
    User  query(int id);

    //更新用户的状态信息
    bool updateState(User user);

    //重置用户的状态信息
    void resetState();


};


#endif