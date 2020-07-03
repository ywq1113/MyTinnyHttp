#pragma

#include <string>

class Utils
{
public:
    Utils() {}
    ~Utils() {}

    int socket_bind(unsinged short int port);

    int setNonBlocking(int fd);
    void setSocketNodelay(int fd);
    void setSocketNoLinger(int fd);

    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);
    void removefd(int epollfd, int fd);
    void modfd(int epollfd, int fd, int ev, int TRIGMode);

public:
    static int *u_pipefd;
    static int u_epollfd;
};