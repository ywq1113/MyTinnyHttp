#ifndef REDIS_H
#define REDIS_H

#include </usr/local/redis/hiredis/hiredis.h>
#include "redisConf.h"
#include <string.h>

using std::string;

class redisConf;

class Redis
{
public:
    Redis() { }
    ~Redis();

    void Connect();
    void releaseConnect();

    void setString(const string & key, const string & value);
    void setString(const string & key, const int & value);
    void setString(const string & key, const float & value);

    void getString(const string & key, string & value);
    void getString(const string & key, int & value);
    void getString(const string & key, float & value);

private:
    void freeReply();
    bool isError();
    void setString(const string &data);

    redisContext *context_;
    redisReply *reply_;
    RedisConf redisConf_;
};

#endif
