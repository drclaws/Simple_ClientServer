#include "TcpSession.hpp"

#if defined(__linux__)
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>

#else
#error System is not supported

#endif

#include <unistd.h>

#include <simple_lib/common.h>

namespace simpleApp
{
    TcpSession::TcpSession(int& epollfd) : ClientSession(epollfd, "TCP")
    {
        //TODO
    }

    session_result TcpSession::init(socket_t masterSocket, uint16_t port)
    {
        struct sockaddr_in address;
        socklen_t len = static_cast<socklen_t>(sizeof(sockaddr_in));

        this->_socket = accept(masterSocket, (sockaddr *)&address, &len);
        if (this->_socket == -1)
        {
            return session_result {session_status::init_socket_setup_fail, errno};
        }
        
        this->_name += std::string(" (") + addressToString(address) + std::string(")");

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
