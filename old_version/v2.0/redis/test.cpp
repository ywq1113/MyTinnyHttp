#include <cstdio>
#include <iostream>
#include "redis.h"
#include "redisConf.h"

using std::string;

void doQuery()
{
    //redisContext *redisConnect(const char *ip, int port);
    //建立连接
    redisContext *rCon_ = redisConnect("127.0.0.1", 6379);
    if(rCon_->err)
    {
        redisFree(rCon_);
        printf("redis connection failur\n");
        return;
    }

    printf("Connect to redisServer Success\n");

    //void *redisCommand(redisContext *c, const char *format, ...);
    //执行命令
    const char *command1 = "set stest1 value1";
    redisReply *rRep_ = (redisReply*)redisCommand(rCon_, command1); 

    if(NULL == rRep_)
    {
        printf("Execut command1 failure\n"); 
        redisFree(rCon_); 
        return; 
    }

    if(!(rRep_->type == REDIS_REPLY_STATUS && strcasecmp(rRep_->str,"OK") == 0))
    {
        printf("Failed to execute command[%s]\n",command1); 
        freeReplyObject(rRep_); 
        redisFree(rCon_); 
        return; 
    }

    freeReplyObject(rRep_); 
    printf("Succeed to execute command[%s]\n", command1);

    redisFree(rCon_);
}

int main()
{
    //doQuery();
    Redis redis_;
    redis_.Connect();

    redis_.setString("name", "ywq");
    redis_.setString("passwd", "123");

    string name_;
    int passwd_;
    redis_.getString("name", name_);
    redis_.getString("passwd", passwd_);
    std::cout << "name : " << name_ << std::endl;
    std::cout << "passwd : " << passwd_ << std::endl;
}


