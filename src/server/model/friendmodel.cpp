#include "friendmodel.hpp"
#include "db.h"





//添加好友关系
void FriendModel::insert(int userid,int friendid){
    //1.组装sql语句
    char sql[1024]={0};

    //组装sql语句部分，这个就是一个sql语句，直接操作mysql的，如果不组装，语句就不对
    sprintf(sql,"insert into friend values(%d,%d)",userid,friendid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }

}

//返回用户好友列表 friendid 
vector<User> FriendModel::query(int userid){
    //1.组装sql语句
   char sql[1024]={0};

   //组装sql语句部分，这个就是一个sql语句，直接操作mysql的，如果不组装，语句就不对
   //friend和user内连接，userid=》friendid
   sprintf(sql,"select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d",userid);

   vector<User> vec;

   MySQL mysql;
   if(mysql.connect()){
        //查询操作
        MYSQL_RES *res =mysql.query(sql);

        if(res !=nullptr){
        
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr){
                User user;
                user.setId(atoi(row[0]));//`atoi(row[0])` 是 C++ 中的一个标准函数，用于将 **C 风格字符串**（`const char*`）转换为 **整数**（`int`）
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }

            //释放资源
            mysql_free_result(res);
            
            return vec;
        }  
   }

   return vec;



}
    
