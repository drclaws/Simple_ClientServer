#include "TcpSession.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>

#include <unistd.h>
#include <errno.h>

#include <simple_lib/common.h>

namespace simpleApp
{
    TcpSession::TcpSession(int& epollfd) : ClientSession(epollfd)
    {
        //TODO
    }

    session_result TcpSession::init(socket_t masterSocket, uint16_t port)
    {
        this->address = new sockaddr_in;
        socklen_t len = static_cast<socklen_t>(sizeof(sockaddr_in));

        this->_socket = accept(masterSocket, (sockaddr *)this->address, &len);
        if (this->_socket == -1)
        {
            delete this->address;
            this->address = nullptr;
            return session_result {session_status::init_socket_setup_fail, errno};
        }
        
        if (set_nonblock(this->_socket) == -1)
        {
            auto err = errno;
            shutdown(this->_socket, SHUT_RDWR);
            close(this->_socket);
            this->_socket = -1;
            return session_result {session_status::init_socket_setup_fail, err};
        }

        struct epoll_event epoll_event;
        epoll_event.data.fd = this->_socket;
        epoll_event.data.ptr = this;
        epoll_event.events = EPOLLIN;

        if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, this->_socket, &epoll_event) == -1)
        {
            auto err = errno;
            shutdown(this->_socket, SHUT_RDWR);
            close(this->_socket);
            this->_socket = -1;
            return session_result {session_status::init_socket_setup_fail, err};
        }

        struct timeval tv;
        tv.tv_sec = 16;
        tv.tv_usec = 0;
        setsockopt(this->_socket, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(tv));
        setsockopt(this->_socket, SOL_SOCKET, SO_SNDTIMEO, (char*) &tv, sizeof(tv));

        return session_result {session_status::init_success, 0};
    }

    session_result TcpSession::proceed(struct epoll_event& epoll_event)
    {
        // TODO
        return session_result {session_status::proceed_msg_send, 0};
    }
}
