#include "http_con.h"
#include "locker.h"
#include "threadpool.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define MAXFD 1024
#define MAXEVENT 1024
/*
proactor模式，主线程负责IO操作，在读写完后，再让工作线程负责后续逻辑
*/
//网络通信种如果一端断开连接了,为了防止另一端进程结束，所以写了SIGPIPE
int addsig(int sig, void(handler)(int))
{  //添加信号捕捉
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    return sigaction(sig, &sa, NULL);
}
//添加文件描述符到epoll中
extern void addfd(int epfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);
int main(int argc, char** argv)
{
    cout << "开始执行" << endl;
    if (argc <= 1)
        exit(-1);

    //对SIGPIPE信号就行处理,防止进程退出
    addsig(SIGPIPE, SIG_IGN);

    //创建线程池
    ThreadPool<http_connection>* thpool = NULL;
    try {
        thpool = new ThreadPool<http_connection>;
    }
    catch (...) {
        exit(-1);
    }

    //保存所有的客户端信息
    http_connection* users = new http_connection[MAXFD];

    //创建监听socket
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);

    //设置端口复用
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    //在socket上绑定服务器的ip地址和端口号
    int port = atoi(argv[1]);  //获取端口号
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
    //开始进行监听
    listen(listenfd, 10);
    cout << "标记1" << endl;
    //创建epoll实例,并创建epoll中的事件数组
    int epfd = epoll_create(1);
    //直接对http_connection中的静态变量初始化
    http_connection::epollfd = epfd;
    epoll_event events[MAXEVENT];
    //添加socket到epoll中
    addfd(epfd, listenfd, false);
    cout << "标记2" << endl;
    while (true) {
        cout << "标记3" << endl;
        // epoll池中检测到socket上发生的新事件，就会被存到events中
        int num_events = epoll_wait(epfd, events, MAXEVENT, -1);
        if (num_events == -1) {
            if (errno == EINTR)  // epoll_wait会被SIGALRM信号中断返回-1
            {
                continue;
            }
            std::cerr << "epoll_wait failed ." << std::endl;
            exit(-1);
        }
        cout << "标记4" << endl;
        //说明有客户端连接进来
        //循环遍历事件数组(这一次的)
        for (int i = 0; i < num_events; i++) {
            cout << "标记5" << endl;
            int socketfd = events[i].data.fd;
            if (socketfd == listenfd) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int connection_fd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
                if (connection_fd == 1) {
                    std::cerr << "accept error" << std::endl;
                }
                if (http_connection::user_count >= MAXFD) {
                    close(connection_fd);
                    continue;
                }
                users[connection_fd].init(connection_fd, client_addr);
                /*
                如果accpet成功，那么其返回值是由内核自动生成的一个全新的描述字，代表与返回客户的TCP连接。
                当有新的客户端连接到来时，先使用accept函数接受连接并创建一个新的套接字描述符confd，该confd里的socketaddr肯定是服务器的地址，客户端的地址存在http_connection对象中然后对其进行处理。
                因为这里的confd是新创建的套接字描述符，它还没有加入到epoll实例中，因此在这里不需要进行epoll_ctl操作。
                接下来，通过调用http_conn::init函数来初始化一个http_conn对象，该对象用于处理客户端连接的HTTP协议请求和响应。
                在http_conn::init函数中，会将新创建的套接字描述符confd添加到epoll实例中，以便之后在事件循环中监视其I/O事件。
                */
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                users[socketfd].close();
            }
            else if (events[i].events & EPOLLIN) {
                if (users[socketfd].read()) {
                    //线程池中加入新任务
                    thpool->addTask(&users[socketfd]);
                }
                else {
                    users[socketfd].close();
                }
            }
            else if (events[i].events & EPOLLOUT) {
                //一次性写完所有数据
                bool ret = users[socketfd].write();
                if (!ret) {
                    users[socketfd].close();
                }
            }
        }
    }
}
