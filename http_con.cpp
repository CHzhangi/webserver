#include "http_con.h"

//向epoll中添加需要监听的描述符
/*epoll 监听事件类型常量定义在<sys / epoll.h> 头文件中，常见的事件类型包括：

    EPOLLIN：表示该文件描述符上有数据可读；
    EPOLLOUT：表示该文件描述符上可以写数据；
    EPOLLERR：表示该文件描述符发生错误；
    EPOLLHUP：表示该文件描述符被挂起；
    EPOLLRDHUP：表示该文件描述符的读端被关闭；
    EPOLLET：表示采用边沿触发的方式；
    EPOLLONESHOT：表示只监听一次事件，需要重新添加事件才能继续监听。

    在ET模式下，如果一个文件描述符一直处于可读状态，但是程序没有及时读取数据，则不会触发多次可读事件，
    只有当文件描述符从不可读状态变为可读状态时才会触发一次可读事件。

    需要注意的是，在ET模式下，事件触发机制是基于状态变化的，程序需要及时读取数据，并在数据读取完毕后重新设置文件描述符的状态，
    以便下一次事件的正确触发。如果程序未能及时读取数据或者未能正确设置文件描述符状态，可能会导致事件无法正确触发，从而影响程序的正常运行。

    在ET模式下，如果将文件描述符设置为阻塞模式进行读操作，当有数据到达时，系统会触发可读事件，并将该事件加入到就绪队列中。
    但是，如果程序在读取数据时由于阻塞等待导致无法立即读取所有的数据，那么下次epoll_wait函数调用时将不会再次返回该事件，
    从而导致程序无法处理该事件。而如果将文件描述符设置为非阻塞模式，当程序在读取数据时遇到无法立即完成的操作时，将立即返回一个错误码，
    程序可以在下一次epoll_wait函数调用时继续处理该事件。

    在网络编程中，可以使用socket函数创建一个套接字，然后通过bind函数将套接字与本地IP地址和端口绑定，最后使用listen函数将套接字转换为监听套接字。
    此时，程序可以通过accept函数接受客户端的连接请求，并得到一个新的套接字文件描述符，用于与客户端进行通信。
    在ET模式下，程序可以使用epoll函数监听新的套接字文件描述符上的可读事件，并在事件触发时，从该套接字文件描述符读取客户端发送的数据。
*/

/*首先使用fcntl函数获取文件描述符的标志位，然后将O_NONBLOCK标志位加入到标志位中，
最后使用fcntl函数重新设置文件描述符的标志位。如果设置成功，函数返回0；否则返回-1，
并设置errno变量指示错误原因。*/
int http_connection::user_count = 0;
int http_connection::epollfd = 0;
void setnoblocking(int fd)
{
    int oldfg = fcntl(fd, F_GETFL);
    oldfg = oldfg | O_NONBLOCK;
    fcntl(fd, F_SETFL, oldfg);
}

void addfd(int epfd, int fd, bool one_shot)
{
    epoll_event event;
    //监听的文件描述符socketfd,epoll实例的文件描述符epollfd
    event.data.fd = fd;
    //监听的事件类型
    event.events = EPOLLIN | EPOLLET;
    //保证同一时刻只有一个线程在处理同一个socket
    // EPOLLONESHOT 表示只监听一次事件，需要重新添加事件才能继续监听
    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    //调用 epoll_ctl 函数将该监听事件添加到 epoll 实例中
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    setnoblocking(fd);
}

void removefd(int epfd, int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
    ::close(fd);
    return;
}
//修改已经添加到epoll实例的被监听文件描述符的监听事件,重置socket上EPOLLONESHOT事件,以确保下次可读时,EPOLLIN事件能被触发
void modifyfd(int epfd, int fd, int events)
{
    epoll_event event;  //重新注册一下事件
    event.data.fd = fd;
    event.events = events | EPOLLONESHOT;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
}

void http_connection::init(int socketfd, sockaddr_in addr)
{
    m_socketfd = socketfd;
    m_address = addr.sin_addr.s_addr;

    int reuse = 1;
    setsockopt(m_socketfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    //添加到epoll池中
    addfd(epollfd, socketfd, EPOLLIN | EPOLLONESHOT | EPOLLET);
    http_connection::user_count++;
}

void http_connection::close()
{
    if (m_socketfd != -1) {
        removefd(http_connection::epollfd, m_socketfd);
        m_socketfd = -1;
        http_connection::user_count--;
    }
}

bool http_connection::read()
{
    cout << "一次性读数据" << endl;
    return true;
    if (read_index >= m_socketfd)
        return false;
    int bytes_read = 0;
    while (true) {
        cout << "一次性读数据" << endl;
        //从文件描述符存到缓冲区
        bytes_read = recv(m_socketfd, read_buf + read_index, READ_BUFFER_SIZE, 0);
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                //没有数据
                break;
            }
            return false;
        }
        else if (bytes_read == 0) {
            //对方关闭
            return false;
        }
        cout << read_buf << endl;
        read_index += bytes_read;
    }
    return true;
}

bool http_connection::write()
{
    cout << "一次性写完" << endl;
    return true;
}

void http_connection::process()
{
    cout << "正在生成响应" << endl;
    return;
}
