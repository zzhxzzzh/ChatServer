#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>


using namespace muduo;
using namespace std;


//暴露一个接口,获取单例对象的接口函数
//静态方法在类外不需要写static这个单词
ChatService * ChatService::instance(){

    static ChatService service;

    return &service;

}


//构造函数，初始化
//注册消息以及对应的回调操作
ChatService::ChatService(){

    //邦迪用户注册函数回调
    _msghandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});

    //绑定用户登录函数回调
    _msghandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});

    _msghandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    //绑定一对一聊天函数回调
    _msghandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});

    //绑定添加好友函数回调
    _msghandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});


    // 群组业务管理相关事件处理回调注册
    _msghandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msghandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msghandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }

}


//处理登录业务  ORM：业务层操作的都是对象，  DAO:数据层做数据，使业务代码和数据代码分开，在业务地方不要存在数据的增删改查
//输入id 和pwd 就可以返回登录成功或者登录失败
void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time){
    //LOG_INFO<<"do login service!!! zzh_zzh";
    
  
    int id=js["id"].get<int>();  //强行转化成int类型
    string pwd=js["password"];


    User user=_userModel.query(id);

    if(user.getId() ==id && user.getPwd()==pwd){

        
        if(user.getState()=="online"){
            //如果用户已经登录，不允许重复登录
            json response;
            //给客户端响应消息
            response ["msgid"]=LOGIN_MSG_ACK;
            
            response["errno"]=2;//表示响应失败
            
            response["errmsg"]="zzh says you:this account is using ,input another!";

            conn->send(response.dump());

        }else{

            //登录成功，记录用户连接信息
            //只需要保护userConnMap的线程安全，数据库的信息有mysql进行保护，无需加锁
            //加个大括号代表作用域，这样加锁安全只保护两行代码
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id,conn});
            }

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id); 

            //登录成功，更新用户状态信息 将state offline 更新成online

            user.setState("online");
            _userModel.updateState(user);//更新用户的状态，将state offline 更新成online
            

            json response;
            //给客户端响应消息
            response ["msgid"]=LOGIN_MSG_ACK;

            response["errno"]=0;//表示响应成功了

            //返回给用户id
            response["id"]=user.getId();

            //返回给用户name
            response["name"]=user.getName();

            //查询该用户是否有离线消息，有的话，装在json，然后带回去
            vector<string> vec=_offlineMsgModel.query(id);
            if(!vec.empty()){
                //vector容器不是空的，就再json中加上，带回去
                response["offlinemsg"]=vec;
                //读取用户的离线消息后，把该用户的所有离线消息清空
                _offlineMsgModel.remove(id);

            }

            //查询该用户的好友信息并返回
            vector<User> userVec=_friendModel.query(id);
            //不空代表有好友
            if(!userVec.empty()){
                vector<string> vec2;
                for(User &user:userVec){
                    json js;
                    js["id"]=user.getId();
                    js["name"]=user.getName();
                    js["state"]=user.getState();
                    vec2.push_back(js.dump());
                }

                response["friends"]=vec2;//这一段主要是把好友信息放在response进行返回给登录的用户
            }


            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }


            conn->send(response.dump());

        }

    }else{

        //该用户不存在，或者用户存在但是密码错误，登录失败
        json response;
        //给客户端响应消息
        response ["msgid"]=LOGIN_MSG_ACK;
        
        response["errno"]=1;//表示响应失败
        
        response["errmsg"]="id or password is invaild!";

        conn->send(response.dump());

    }




}

//处理注册业务 // name password
void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp time){
    //LOG_INFO<<"do reg service!!! zzh_zzh";
    string name=js["name"];
    string pwd=js["password"];


    User user;
    user.setName(name);
    user.setPwd(pwd);

    //进行新用户的插入，这里userModel.insert是直接调用mysql语句
    bool state= _userModel.insert(user);

    if(state){
        //注册成功
        json response;
        //给客户端响应消息
        response ["msgid"]=REG_MSG_ACK;

        response["errno"]=0;//表示响应成功了

        //返回给用户id
        response["id"]=user.getId();

        conn->send(response.dump());

    }else{
        //注册失败
        json response;
        //给客户端响应消息
        response ["msgid"]=REG_MSG_ACK;

        response["errno"]=1;//表示响应失败

        conn->send(response.dump());
    }

}



 //获取消息对应的处理器
 //using MsgHandler=std::function<void(const TcpConnectionPtr &coon,json &js,Timestamp)>;
 //这里的MsgHandler 是一个 回调函数类型，它表示的是一个函数指针或函数对象。这个回调函数会被用来处理不同类型的消息。简单理解为int
 MsgHandler ChatService::getHandler(int msgid){

    //记录错误日志，msgid没有对应的事件处理回调
    auto it=_msghandlerMap.find(msgid);
    if(it==_msghandlerMap.end()){

        //记录msgid找不到相应的处理器
       //LOG_ERROR << "msgid" << msgid << "can not find handler!";

       //找不到相应的处理器就返回一个默认的处理器，空操作
       return [=](const TcpConnectionPtr &conn,json &js,Timestamp time){
            LOG_ERROR << "msgid" << msgid << "can not find handler!";
       };


    }else{

        return _msghandlerMap[msgid];
    }

 }


//处理客户端异常退出的函数的具体实现
void ChatService::clientCloseException(const TcpConnectionPtr &conn){

    User user;//创建一个User实例对象处理
    //线程安全
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it=_userConnMap.begin();it!=_userConnMap.end();++it){
            //相等代表找到异常退出的coon，即找到异常退出的客户端
            if(it->second == conn){
                user.setId(it->first);//将客户端用户的id传给实例化出来的user，方便其更新用户的状态信息
                //从_userConnMap中删除用户的连接信息
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId()); 

    //更新用户的状态信息
    //if是为了确定是一个有效的用户
    if(user.getId()!=-1){
        user.setState("offline");
        _userModel.updateState(user);//根据指定的id将用户状态信息更新

    }

}


//一对一聊天业务
/*
    {"msgid":1,"id":13,"password":"123456")

    msgid
    id:1  //自己的id
    name:"zhang san" //自己的名字
    to:3  //对方的id
    msg:"xxxxxx"
*/
void ChatService::oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time){

    int toid=js["toid"].get<int>();

  
    //加锁
    {
        lock_guard<mutex> lock(_connMutex);
        auto it=_userConnMap.find(toid);

        if(it != _userConnMap.end()){
            //toid在线，去转发消息，转发消息
            //服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线 
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    //toid不在线，存储离线消息
    _offlineMsgModel.insert(toid,js.dump());


}




//服务器异常，业务重置方法,解决的是服务器ctrl + c退出
void ChatService::reset(){

    //把所有用户的状态online设置成offline
    _userModel.resetState();

}



//添加好友业务 msg id friend
void ChatService::addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time){
    
    int userid=js["id"].get<int>();
    int friendid=js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userid,friendid);

}


// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        // auto it=_userConnMap.find(id);

        // if(it != _userConnMap.end()){
        //      // 转发群消息
        //     it->second->send(js.dump());
        // }else{
        //     // 存储离线群消息
        //     _offlineMsgModel.insert(id, js.dump());
        // }

        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线 
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}



// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid); 

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}



// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}


