#pragma once

#include "UdpSession.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>

#include <simple_lib/message.h>
#include <simple_lib/common.h>

namespace simpleApp
{
    UdpSession::UdpSession(int& epollfd) : ClientSession(epollfd)
    {
        // TODO
    }

    UdpSession::~UdpSession()
    {
        if (this->timerfd != -1)
        {
            epoll_ctl(this->epollfd, EPOLL_CTL_DEL, this->timerfd, 0);
            close(this->timerfd);
        }
        if (this->_socket != -1)
        {
            const uint8_t buff = static_cast<const uint8_t>(udp_header::server_conndown);
            send(this->_socket, &buff, sizeof(buff), 0);
        }
    }

    session_result UdpSession::init(socket_t masterSocket, uint16_t port)
    {
        const size_t buffCheckLength = sizeof(udp_header) + 1;
        uint8_t msgBuff[buffCheckLength];
        this->address = new sockaddr_in;
        socklen_t len = static_cast<socklen_t>(sizeof(sockaddr_in));

        auto len = recvfrom(masterSocket, msgBuff, buffCheckLength, 0, (sockaddr *)this->address, &len);

        if (len == -1)
        {
            delete this->address;
            this->address = nullptr;
            return session_result {session_status::init_udp_listener_fail, errno};
        }
        
        if (len != sizeof(udp_header))
            return session_result {session_status::init_udp_wrong_length, 0};
        
        if (*(reinterpret_cast<udp_header *>(msgBuff)) != udp_header::client_connup)
            return session_result {session_status::init_udp_wrong_header, 0};

        auto initSocket = [this, port]()
        {
            this->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
            if (this->timerfd == -1)
                return session_result {session_status::init_udp_timer_fail, 0};
            
            if (!this->timerReset())
            {
                auto err = errno;
                close(this->timerfd);
                this->timerfd = -1;
                return session_result {session_status::init_udp_timer_fail, err};
            }
            
            if (epoll_ctl(this->epollfd, EPOLL_CTL_DEL, this->timerfd, 0) == -1)
            {
                auto err = errno;
                close(this->timerfd);
                this->timerfd = -1;
                return session_result {session_status::init_udp_timer_fail, err};
            }
                    
            this->_socket = socket(AF_INET, SOCK_DGRAM, 0);
            if (this->_socket == -1)
            {
                return session_result {session_status::init_socket_setup_fail, errno};
            }

            if (set_nonblock(this->_socket) == -1)
            {
                auto err = errno;
                close(this->_socket);
                this->_socket = -1;
                return session_result {session_status::init_socket_setup_fail, err};
            }

            int optval = 1;
            if (setsockopt(this->_socket, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval))) == -1)
            {
                auto err = errno;
                close(this->_socket);
                this->_socket = -1;
                return session_result {session_status::init_socket_setup_fail, err};
            }
            
            if (connect(this->_socket, (sockaddr *)this->address, sizeof(sockaddr_in)) == -1)
            {
                auto err = errno;
                close(this->_socket);
                this->_socket = -1;
                return session_result {session_status::init_socket_setup_fail, err};
            }

            struct sockaddr_in socketBindAddress;
            socketBindAddress.sin_addr.s_addr = htonl(INADDR_ANY);
            socketBindAddress.sin_port = htons(port);
            socketBindAddress.sin_family = AF_INET;

            if (bind(this->_socket, (sockaddr *)&socketBindAddress, sizeof(socketBindAddress)) == -1)
            {
                auto err = errno;
                shutdown(this->_socket, SHUT_RDWR);
                close(this->_socket);
                this->_socket = -1;
                return session_result {session_status::init_socket_setup_fail, err};
            }

            return session_result {session_status::init_success, 0};
        };
        
        auto result = initSocket();

        if(result.status == session_status::init_success)
        {
            const uint8_t header = static_cast<const uint8_t>(udp_header::accept_connstart);
            send(this->_socket, &header, sizeof(header), 0);
        }
        else
        {
            const uint8_t header = static_cast<const uint8_t>(udp_header::err_connstart);
            sendto(masterSocket, &header, sizeof(header), 0, (sockaddr *)this->address, sizeof(sockaddr_in));
        }

        return result;
    }

    session_result UdpSession::proceed(struct epoll_event& epoll_event)
    {
        /*
        if (this->_socket == -1)
        {
            // TODO не рассматриваем
        }

        if (epoll_event.data.fd == this->timerfd)
        {
            struct itimerspec timerData;
            if (timerfd_gettime(this->timerfd, &timerData) == -1)
            {
                
            }

            if (timerData.it_value.tv_sec == 0 && timerData.it_value.tv_nsec == 0)
            {
                
            }
        }
        else if (epoll_event.data.fd == this->_socket)
        {
            uint8_t msgBuff[MESSAGE_MAX_BUFFER];
            auto len = recv(this->_socket, msgBuff, MESSAGE_MAX_BUFFER, 0);
            if (len == -1)
            {

            }
            return ;
        }
        else
        {
            return ;
        }*/
        
        return session_result {session_status::proceed_msg_send, 0};
    }

    bool UdpSession::timerReset()
    {
        struct itimerspec timerspec;
        timerspec.it_interval.tv_sec = 0;
        timerspec.it_interval.tv_nsec = 0;
        timerspec.it_value.tv_sec = WAIT_TIMEOUT_SEC;
        timerspec.it_value.tv_nsec = 0;

        return timerfd_settime(this->timerfd, 0, &timerspec, nullptr) == 0;
    }
}
