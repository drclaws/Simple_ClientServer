#include "SessionTcp.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>

#include <unistd.h>

#include <simple_lib/common.h>

#include "message.h"

namespace simpleApp
{
    SessionTcp::SessionTcp(int epollfd) : SessionServer(epollfd, std::string("TCP"))
    {
        
    }

    SessionTcp::~SessionTcp()
    {

    }

    session_result SessionTcp::init(socket_t masterSocket)
    {
        sockaddr_in address;
        socklen_t len = static_cast<socklen_t>(sizeof(sockaddr_in));

        this->_socket = accept(masterSocket, (sockaddr *)&address, &len);
        if (this->_socket == -1)
        {
            return session_result(session_status::init_socket_setup_fail, errno);
        }
        
        this->_name += std::string(" (") + addressToString(address) + std::string(")");

        if (set_nonblock(this->_socket) == -1)
        {
            auto err = errno;
            shutdown(this->_socket, SHUT_RDWR);
            close(this->_socket);
            this->_socket = -1;
            return session_result(session_status::init_socket_setup_fail, err);
        }

        epoll_event epoll_event_setup;
        epoll_event_setup.data.ptr = this;
        epoll_event_setup.events = EPOLLIN;

        if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, this->_socket, &epoll_event_setup) == -1)
        {
            auto err = errno;
            shutdown(this->_socket, SHUT_RDWR);
            close(this->_socket);
            this->_socket = -1;
            return session_result(session_status::init_socket_setup_fail, err);
        }

        timeval tv;
        tv.tv_sec = 16;
        tv.tv_usec = 0;
        setsockopt(this->_socket, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(tv));
        setsockopt(this->_socket, SOL_SOCKET, SO_SNDTIMEO, (char*) &tv, sizeof(tv));

        return session_result(session_status::init_success);
    }

    session_result SessionTcp::proceed()
    {
        uint8_t buffer[MESSAGE_MAX_BUFFER];
        auto len = recv(this->_socket, buffer, MESSAGE_MAX_BUFFER, 0);
        if (len == -1)
            return session_result(session_status::proceed_recv_fail, errno);
        else if (len == 0)
        {
            this->sessionClose();
            return session_result(session_status::proceed_disconnect);
        }
        else if (static_cast<size_t>(len) <= sizeof(msg_headers))
        {
            this->sendMessage(msg_headers::incorrect_msg);
            return session_result(session_status::proceed_tcp_wrong_size);
        }
        
        if(*reinterpret_cast<msg_headers*>(buffer) != msg_headers::client_msg)
        {
            this->sendMessage(msg_headers::incorrect_msg);
            return session_result(session_status::proceed_wrong_header);
        }
        
        char buff[MESSAGE_MAX_BUFFER - sizeof(msg_headers)];
                    
        auto sendLen = proceedMsg(reinterpret_cast<char*>(buffer + static_cast<ptrdiff_t>(sizeof(msg_headers))), len - sizeof(msg_headers), buff);

        if (this->sendMessage(msg_headers::server_msg, buff, sendLen) == -1)
            return session_result(session_status::proceed_send_fail, errno);

        return session_result(session_status::proceed_msg_send);
    }
}
