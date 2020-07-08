#ifndef REDISCONF_H
#define REDISCONF_H

#include <string>

using std::string;

class RedisConf
{
public:
    RedisConf();

    void getConf();
    
    string getIP();
    
    int getPort();

private:
    string ip_;
    int port_;
};

#endif