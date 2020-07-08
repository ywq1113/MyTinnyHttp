#include "redisConf.h"

#include <json/json.h>
#include <fstream>
#include <cstdio>

using std::ifstream;

RedisConf::RedisConf()
{
    getConf();
}

void RedisConf::getConf()
{
    ifstream ifs;
    ifs.open("redisConf.json");

    if(!ifs.good()) /*文件流是否正常*/
    {
        fprintf(stderr, "RedisConf open redisConf.json error\n");
        exit(1);
    }

    Json::Value root;
    Json::Reader reader;
    if(!reader.parse(ifs, root, false))
    {
        fprintf(stderr, "RedisConf json reader error\n");
        exit(1);
    }

    ip_ = root["ip"].asString();
    port_ = root["Port"].asInt();
    ifs.close();
}

string RedisConf::getIP()
{
    return ip_;
}

int RedisConf::getPort()
{
    return port_;
}
