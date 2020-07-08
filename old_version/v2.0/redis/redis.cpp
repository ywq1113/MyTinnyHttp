#include "redis.h"

using std::stringstream;

#define SETSTRING(key, value) \
    stringstream ss;\
    ss << "SET " << key << " " << value;\
    string s;\
    getline(ss, s);\
    setString(s);\

void Redis::~Redis()
{
    releaseConnect();
}

void Redis::Connect()
{
    string ip_ = redisConf_.getIP();
    int port_ = redisConf_.getPort();
    context_ = ::redisConnect(ip_.c_str(), port_);
    char str[50];
    snprintf(str, sizeof(str), "redis connect on host: ", ip_);
    printf("%s ", str);
    printf("port: %d\n", port_);

    if(context_ || context_.err)
    {
        redisFree(context_);
        fprintf(stderr, "redis connection failur\n");
        exit(1);
    }

    printf("redis connect success\n");
}


void Redis::releaseConnect()
{
    freeReply(reply_); 
    ::redisFree(context_);
    printf("release redis connection success\n");
}

bool Redis::isError()
{
    if(NULL == reply_)
    {
        releaseConnect();
        Connect();
        //fprintf(stderr, "Redis execute command failure\n");
        return true;
    }
    return false;
}

void Redis::setString(const string &data)
{
    freeReply();
    reply_ = (redisReply*)redisCommand(context_, data.c_str());
    if(isError())
    {
        if(!(reply_->type == REDIS_REPLY_STATUS && strcasecmp(_reply->str,"OK") == 0))
        {
            printf("Failed to execute SET(string)\n");
        }    
    }
}

void Redis::setString(const string &key, const string &value)
{
    SETSTRING(key, value);
}

void Redis::setString(const string &key, const int &value)
{
    SETSTRING(key, value);
}

void Redis::setString(const string &key, const float &value)
{
    SETSTRING(key, value);
}

void Redis::getString(const string& key)
{
    freeReply();
    reply_ = (redisReply*)redisCommand(context_, key.c_str());
}

void Redis::getString(const string &key, string &value)
{
    getString(key);
    if(!isError() && reply_->type == REDIS_REPLY_STRING)
    {
        value = reply_->str /* char* str */;
    }
}

void Redis::getString(const string &key, int &value)
{
    getString(key);
    if(!isError() && reply_->type == REDIS_REPLY_STRING)
    {
        value = ::atoi(reply_->str /* char* str */);
    }
}

void Redis::getString(const string &key, float &value)
{
    getString(key);
    if(!isError() && reply_->type == REDIS_REPLY_STRING)
    {
        value = ::atof(reply_->str /* char* str */);
    }
}

void Redis::freeReply()
{
    if(reply_)
    {
        ::freeReplyObject(reply_);
        reply_ = NULL;
    }
}