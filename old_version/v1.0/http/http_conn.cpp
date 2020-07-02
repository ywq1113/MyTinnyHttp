#include "http_conn.h"

const char* ok_200_title = "OK\n";
const char* error_400_title = "Bad Request\n";
const char* error_400_form  = "Your request has bad syntax or is inherently impossible to satisfy\n";
const char* error_403_title = "Forbidden\n";
const char* error_403_form  = "You don't have permession to get file from this server\n";
const char* error_404_title = "Not Found\n";
const char* error_404_form  = "The requested file was not found on this server\n";
const char* error_500_title = "Internal Error\n";
const char* error_500_form  = "There was an unusual problem serving the requested file\n";
const char* doc_root = "/home/project/LoginSystem/htdocs";


int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);  //F_GETFL获取fd的flag
    int new_option = old_option | O_NONBLOCK;  //设置新的flag
    fcntl(fd, F_SETFL, new_option);  //把新的flag赋给fd
    return old_option;
}

void addfd(int epollfd, int fd, bool one_shot){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN || EPOLLET || EPOLLRDHUP;
    if(one_shot){
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

/*从epollfd事件表上删除fd注册的事件，并关闭fd*/
void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void modfd(int epollfd, int fd, int ev){
    epoll_event event;
    event.data.fd = fd;
    event.events |= ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

void http_conn::close_conn(bool real_close)
{
        if(real_close && (m_sockfd!=-1)){
            removefd(m_epollfd, m_sockfd);
            m_sockfd = -1;
            m_user_count--;
        }
}

void http_conn::init(int sockfd, const sockaddr_in& addr)
{
    m_sockfd = sockfd;
    m_address = addr;
    /*避免time_wait,socket重用*/
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    addfd(m_epollfd, sockfd, true);
    m_user_count++;
    init();
}

void http_conn::init()
{
    m_check_stat = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    memset(m_write_buf,'\0',WRITE_BUFF_SIZE);
    memset(m_read_buf,'\0', READ_BUFF_SIZE);
    memset(m_real_file,'\0',FILENAME_LEN);
}

http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    for(;m_checked_idx<m_read_idx;++m_checked_idx){
        temp = m_read_buf[m_checked_idx];
        if(temp=='\r'){
            if(m_checked_idx+1 == m_read_idx){
                return LINE_OPEN;
            }
            else if(m_read_buf[m_checked_idx+1]=='\n'){
                m_read_buf[m_checked_idx++] = '\0';
                /*m_read_buf[m_checked_idx++] = '\0';*/
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if(temp=='\n'){
            if((m_checked_idx>1) && m_read_buf[m_checked_idx-1]=='\r'){
                m_read_buf[m_checked_idx-1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

bool http_conn::read()
{
    if(m_read_idx>=READ_BUFF_SIZE){
        return false;
    }
    int bytes_read = 0;
    while(true){
        bytes_read = recv(m_sockfd, m_read_buf+m_read_idx, READ_BUFF_SIZE-m_read_idx,0);
        if(bytes_read==-1){
            if(errno==EAGAIN || EWOULDBLOCK)
                break;
            return false;
        }
        else if(bytes_read==0)
            return false;
        m_read_idx += bytes_read;
    }
    return true;
}

http_conn::HTTP_CODE http_conn::parse_request_line(char* temp)
{
    char* m_url = strpbrk(temp," \t");
    /*如果请求行中没有空白字符或\t字符，那么请求有问题*/
    if(!m_url){
        return BAD_REQUEST;
    }
    *m_url++ = '\0'; /*把m_url之前的部分用'\0'结束*/
    
    /*只支持GET和POST方法*/
    char* method = temp;
    if(strcasecmp(method, "GET")==0)
    {
        m_method = GET;
    }
    else if(strcasecmp(method,"POST")==0){
        m_method = POST;
    }
    else
        return BAD_REQUEST;
    m_url += strspn(m_url, "\t");
    
    /*仅仅支持HTTP1.1*/
    char* version = strpbrk(m_url, " \t");
    if(!version){
        return BAD_REQUEST;
    }
    *version++ = '\0'; 
    version += strspn(version, " \t");
    if(strcasecmp(version, "HTTP/1.1")!=0){
        return BAD_REQUEST;
    }
    
    /*检查m_url是否合法*/
    if(strncasecmp(m_url, "http://",7)==0){
        m_url += 7;
        m_url = strchr(m_url, '/');  /*strchr返回首次出现char c位置的指针，即m_url指向'/' */
    }
    
    if(!m_url || m_url[0]!='/'){
        return BAD_REQUEST;
    }
    printf("The request m_url is %s\n", m_url);
    /*HTTP请求行处理完毕，状态转移到头部字段分析*/
    m_check_stat = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_headers(char* temp)
{
    /*遇到空行，说明得到了一个正确的HTTP请求*/
    if(temp[0]=='\0'){
        if(m_content_length!=0){
            m_check_stat = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if(strncasecmp(temp, "Connection:",11)==0){
        temp += 11;
        temp += strspn(temp, " \t");
        if(strcasecmp(temp,"keep-live")==0)
            m_linger = true;
    }
    else{
        printf("can not handle this header\n");
        return BAD_REQUEST;
    }
    return NO_REQUEST;
}
/*只是单纯地判断是否完整读取content*/
http_conn::HTTP_CODE http_conn::parse_content(char* temp){
    if(m_read_idx>=(m_content_length+m_checked_idx)){
        temp[m_content_length] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

/*主状态机*/
http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* temp = 0;
    while(((m_check_stat == CHECK_STATE_CONTENT)&&(line_status == LINE_OK)) || 
            ((line_status=parse_line())==LINE_OK) )
    {
        temp = get_line(); /*目前没看到此函数实现地方*/
        m_start_line = m_checked_idx;
        printf("got 1 http line: %s\n", temp);
        switch(m_check_stat){
            case CHECK_STATE_REQUESTLINE:
            {
                ret = parse_request_line(temp);
                if(ret==BAD_REQUEST){
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                ret = parse_headers(temp);
                if(ret==BAD_REQUEST)
                    return BAD_REQUEST;
                else if(ret==GET_REQUEST)
                    return do_request();
                break;
            }
            case CHECK_STATE_CONTENT:
            {
                ret = parse_content(temp);
                if(ret==GET_REQUEST){
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}

/*得到完整正确的请求后，分析文件属性，存在且不是目录，对所有用户可见，使用mmap系统映射*/
http_conn::HTTP_CODE http_conn::do_request()
{
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    strncpy(m_real_file+len, m_url, FILENAME_LEN-len-1);
    if(stat(m_real_file, &m_file_st)<0){
        return NO_RESOURSE;
    }
    if(!(m_file_st.st_mode&S_IROTH))
        return FORBIDDEN_REQUEST;
    if(S_ISDIR(m_file_st.st_mode))
        return BAD_REQUEST;
    
    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char*)mmap(0,m_file_st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

void http_conn::unmap(){
    if(m_file_address){
        munmap(m_file_address, m_file_st.st_size);
        m_file_address = 0;
    }
}
/*读缓冲区里面读数据*/
bool http_conn::write(){
    int tmp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = m_write_idx;
    if(bytes_to_send==0){
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }
    while(1){
        tmp = writev(m_sockfd, m_iv, m_iv_count);
        /*如果TCP写缓冲没有空间，等待下一轮EPOLLOUT*/
        if(tmp<=-1){
            if(errno==EAGAIN){
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }
        bytes_to_send -= tmp;
        bytes_have_send += tmp;
        if(bytes_to_send<=bytes_have_send){
            unmap();
            if(m_linger){
                init();
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                return true;
            }
        }
        else{
            modfd(m_epollfd, m_sockfd, EPOLLIN);
            return false;
        }
    }
}
/*往写缓冲区写入数据*/
bool http_conn::add_response(const char* format,...){
    if(m_write_idx>=WRITE_BUFF_SIZE){
        return false;
    }
    va_list arg_list;
    va_start(arg_list, format);

    int len = vsnprintf(m_write_buf+m_write_idx, WRITE_BUFF_SIZE-1-m_write_idx, format, arg_list);
    if(len>=(WRITE_BUFF_SIZE-1-m_write_idx)){
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);
    return true;
}

bool http_conn::add_status_line(int status, const char* title){
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_len){
    add_content_length(content_len);
    add_linger();
    add_blank_line();
}

bool http_conn::add_content_length(int content_len){
    return add_response("Content-length: %d\r\n", content_len);
} 

bool http_conn::add_linger(){
    return add_response("Connection: %s\r\n", (m_linger==true)?"keep-alive":"close");
}
    
bool http_conn::add_blank_line(){
    return add_response("%s","\r\n");
}

bool http_conn::add_content(const char* content){
    return add_response("%s", content);
}

/*根据服务器处理HTTP请求结果，决定返回给客户端的内容*/
bool http_conn::process_write(HTTP_CODE ret){
    switch(ret){
        case INTERNAL_ERROR:
        {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if(!add_content(error_500_form))
                return false;
            break;
        }
        case BAD_REQUEST:
        {
            add_status_line(400, error_400_title);
            add_headers(strlen(error_400_form));
            if(!add_content(error_400_form))
                return false;
            break;
        }
        case NO_RESOURSE:
        {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if(!add_content(error_404_form))
                return false;
            break;
        }
        case FILE_REQUEST:
        {
            add_status_line(200, ok_200_title);
            if(m_file_st.st_size!=0){
                add_headers(m_file_st.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_st.st_size;
                m_iv_count = 2;
                return true;
            }
            else{
                const char* ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if(!add_content(ok_string)){
                    return false;
                }
            }
        }
        default:
        {
            return false;
        }
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    return true;
}
/*线程池中的工作线程调用，处理http请求的入口函数*/
void http_conn::process(){
    HTTP_CODE read_ret = process_read();
    if(read_ret==NO_REQUEST){
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }
    bool write_ret = process_write(read_ret);
    if(!write_ret){
        close_conn();
    }
    modfd(m_epollfd,m_sockfd,EPOLLOUT);
}

