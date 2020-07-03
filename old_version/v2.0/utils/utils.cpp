#include "utils.h"

int utils::socket_bind(unsinged short int port)
{
    if(port < 0 || port > 65535) return -1;
    int listenfd_ = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd_ >= 0);

    //端口可以复用
    int flag = 1;
    if(setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
    {
        close(listenfd_);
        return -1;
    }

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);
    
    if(bind(listenfd_, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        close(listenfd_);
        return -1;
    }
    if(listen(listenfd_, 2048) == -1)
    {
        close(listenfd_);
        return -1;
    }

    //监听失败返回-1
    if(listenfd_ == -1) 
    {
        close(listenfd_);
        return -1;
    }

    return listenfd_;
}

int utils::setNonBlocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void utils::setSocketNodelay(int fd)
{
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
}

void utils::setSocketNoLinger(int fd)
{
    struct linger linger_;
    linger_.l_onoff = 1;
    linger_.l_linger = 30;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&linger_,
             sizeof(linger_));
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void utils::modfd(int epollfd, int fd, int ev, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

//从内核时间表删除描述符
void utils::removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}