#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include "locker.h"

class http_conn{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFF_SIZE = 2048;
    static const int WRITE_BUFF_SIZE = 1024;
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, 
                 TRACE, OPTION, CONNECT, PATH};
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0,
                       CHECK_STATE_HEADER, CHECK_STATE_CONTENT,
                       CHECK_STATE_CONNECT };
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST,
                     NO_RESOURSE, FORBIDDEN_REQUEST, FILE_REQUEST,
                     INTERNAL_ERROR, CLOSED_CONNECTION};
    /*行的读取状态*/
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };
public:
    http_conn(){}
    ~http_conn(){}

    /*初始化新接受的连接*/
    void init(int sockfd, const sockaddr_in& addr);
    /*关闭连接*/
    void close_conn(bool real_close = true);
    /*处理客户请求*/
    void process();
    /*非阻塞读操作*/
    bool read();
    /*非阻塞写操作*/
    bool write();
    
private:
    void init(); /*初始化连接*/
    HTTP_CODE process_read(); /*解析http请求*/
    bool process_write(HTTP_CODE ret); /*填充http应答*/
    /*process_read调用分析请求*/
    HTTP_CODE parse_request_line(char* temp);
    HTTP_CODE parse_headers(char* temp);
    HTTP_CODE parse_content(char* temp);
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();
    /*process_write调用填充响应*/
    void unmap();
    bool add_response(const char* format,...);
    bool add_content(const char* content);
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
    
public:
    static int m_epollfd;
    static int m_user_count;  /*统计用户数量*/
    
private:
    int m_sockfd;  /*http连接的socket*/
    sockaddr_in  m_address; 
    char m_read_buf[READ_BUFF_SIZE];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;
    char m_write_buf[WRITE_BUFF_SIZE];
    int m_write_idx;
    
    /*主状态机当前所处状态*/
    CHECK_STATE m_check_stat;
    METHOD m_method;
    
    /*客户端请求的文件路径*/
    char m_real_file[FILENAME_LEN];
    char* m_url;
    char* m_version;
    char* m_host;
    /*请求的消息体的长度*/
    int m_content_length;
    /*http请求是否保持连接*/
    bool m_linger;
    
    /*客户请求的文件被mmap到内存中的起始位置*/
    char* m_file_address;
    struct stat m_file_st;
    /*writev来执行写操作，m_iv_count表示被写内存块数量*/
    struct iovec m_iv[2];
    int m_iv_count;
};

#endif  

