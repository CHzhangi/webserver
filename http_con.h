#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "locker.h"
#include "threadpool.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <map>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

/*
主线程负责的是:

    接收客户端连接请求 (通过 accept() 接口)
    从已经建立的连接套接字中读取数据 (通过 recv() 接口)
    调用业务处理函数处理读取到的数据
    向已连接的套接字写入响应数据(通过 send() 接口)
*/
class http_connection {
public:
    static int epollfd;  //所有socket上的事件都被注册到同一个epoll中
    static int user_count;
    static const int READ_BUFFER_SIZE = 1024;
    static const int WRITE_BUFFER_SIZE = 1024;
    void close();
    bool read();
    bool write();
    void process();  //处理客户端请求
    void init(int socketfd, sockaddr_in addr);
    // void addfd(int epollfd, int fd, bool one_shot);
    // void removefd(int epollfd, int fd);
    // void modifyfd(int epollfd, int fd, int events);

private:
    int m_socketfd;  //该http连接的socket
    int m_address;   //客户端地址
    char read_buf[READ_BUFFER_SIZE];
    int read_index;
    char write_buf[WRITE_BUFFER_SIZE];
    //
};

#endif