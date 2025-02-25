#include "offlinemessagemodel.hpp"
#include "db.h"



//存储用户的离线消息
void OfflineMsgModel::insert(int userid,string msg){

     //1.组装sql语句
     char sql[1024]={0};

     //组装sql语句部分，这个就是一个sql语句，直接操作mysql的，如果不组装，语句就不对
     sprintf(sql,"insert into offlinemessage values('%d','%s')",userid,msg.c_str());

     MySQL mysql;
     if(mysql.connect()){
        //mysql更新
        mysql.update(sql);

     }
 

}

//删除用户的离线消息
void OfflineMsgModel::remove(int userid){
      //1.组装sql语句
      char sql[1024]={0};

      //组装sql语句部分，这个就是一个sql语句，直接操作mysql的，如果不组装，语句就不对
      sprintf(sql,"delete from offlinemessage where userid=%d",userid);
 
      MySQL mysql;
      if(mysql.connect()){
         //mysql更新
         mysql.update(sql);
 
      }

}

//查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid){

   //1.组装sql语句
   char sql[1024]={0};

   //组装sql语句部分，这个就是一个sql语句，直接操作mysql的，如果不组装，语句就不对
   sprintf(sql,"select message from offlinemessage where userid=%d",userid);

   vector<string> vec;

   MySQL mysql;
   if(mysql.connect()){
        //查询操作
        MYSQL_RES *res =mysql.query(sql);

        if(res !=nullptr){
        
            //把userid用户的所有离线消息放入到vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr){
                vec.push_back(row[0]);
            }

            //释放资源
            mysql_free_result(res);
            

            return vec;

        }  
   }

   return vec;
}