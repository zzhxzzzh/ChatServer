#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

//User表的ORM类
class User{
 public:
    //提供一些方法

    //构造函数，初始化
    User(int id=-1,string name="",string pwd="",string state="offline"){
        this->id=id;
        this->name=name;
        this->password=pwd;
        this->state=state;
    }

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPwd(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPwd() { return this->password; }
    string getState() { return this->state; }

 protected:

    int id;//用户id
    string name;//用户名称
    string password;//用户密码
    string state;//当前登录状态
};




#endif