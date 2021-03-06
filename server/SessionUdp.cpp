#include "SessionUdp.hpp"

#include <cstring>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>

#include <simple_lib/common.h>

#include "message.h"

namespace simpleApp
{
    SessionUdp::SessionUdp(int epollfd) : SessionServer(epollfd, std::string("UDP"))
    {
        
    }

    SessionUdp::~SessionUdp()
    {
        if (this->timerfd != -1)
        {
            epoll_ctl(this->epollfd, EPOLL_CTL_DEL, this->timerfd, 0);
            close(this->timerfd);
        }
        if (this->_socket != -1)
        {
            if (this->isTimeout)
            {
                this->sendMessage(msg_headers::server_timeout);
            }
            else 
            {
                this->sendMessage(msg_headers::server_conndown);
            }
        }
    }
    
    session_result SessionUdp::init(socket_t masterSocket)
    {
        const size_t buffCheckLength = sizeof(msg_headers) + 1;
        uint8_t msgBuff[buffCheckLength];
        sockaddr_in address;
        socklen_t addressLen = static_cast<socklen_t>(sizeof(sockaddr_in));

        auto len = recvfrom(masterSocket, msgBuff, buffCheckLength, 0, (sockaddr *)&address, &addressLen);

        auto sendInitError = [masterSocket, &address]()
        {
            const msg_headers buff = msg_headers::err_connstart;
            sendto(masterSocket, &buff, sizeof(buff), 0, (sockaddr *)&address, sizeof(address));
        };

        if (len == -1)
        {
            return session_result(session_status::init_listener_fail, errno);
        }
        
        this->_name += std::string(" (") + addressToString(address) + std::string(")");

        if (len != sizeof(msg_headers))
        {
            sendInitError();
            return session_result(session_status::init_udp_wrong_length);
        }

        if (*(reinterpret_cast<msg_headers *>(msgBuff)) != msg_headers::req_connstart)
        {
            sendInitError();
            return session_result(session_status::init_udp_wrong_header);
        }

        auto initSocket = [this, &address]()
        {
            this->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
            
            if (this->timerfd == -1)
                return session_result(session_status::init_udp_timer_fail);
            
            if (!this->timerReset())
            {
                auto result = session_result(session_status::init_udp_timer_fail, errno);
                close(this->timerfd);
                this->timerfd = -1;
                return result;
            }
            
            epoll_event timerEvent;
            timerEvent.events = EPOLLIN;
            timerEvent.data.fd = this->timerfd;
            timerEvent.data.ptr = this;

            if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, this->timerfd, &timerEvent) == -1)
            {
                auto result = session_result(session_status::init_udp_timer_fail, errno);
                close(this->timerfd);
                this->timerfd = -1;
                return result;
            }
            
            this->_socket = socket(AF_INET, SOCK_DGRAM, 0);
            if (this->_socket == -1)
            {
                return session_result(session_status::init_socket_setup_fail, errno);
            }

            if (set_nonblock(this->_socket) == -1)
            {
                auto result = session_result(session_status::init_socket_setup_fail, errno);
                close(this->_socket);
                this->_socket = -1;
                return result;
            }

            int optval = 1;
            if (setsockopt(this->_socket, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval))) == -1)
            {
                auto result = session_result(session_status::init_socket_setup_fail, errno);
                close(this->_socket);
                this->_socket = -1;
                return result;
            }

            sockaddr_in socketBindAddress;
            bzero(&socketBindAddress, sizeof(socketBindAddress));
            socketBindAddress.sin_addr.s_addr = htonl(INADDR_ANY);
            socketBindAddress.sin_port = htons(PUBLIC_PORT);
            socketBindAddress.sin_family = AF_INET;

            if (bind(this->_socket, (sockaddr *)&socketBindAddress, sizeof(socketBindAddress)) == -1)
            {
                auto result = session_result(session_status::init_socket_setup_fail, errno);
                shutdown(this->_socket, SHUT_RDWR);
                close(this->_socket);
                this->_socket = -1;
                return result;
            }
            
            if (connect(this->_socket, (sockaddr *)&address, sizeof(sockaddr_in)) == -1)
            {
                auto result = session_result(session_status::init_socket_setup_fail, errno);
                close(this->_socket);
                this->_socket = -1;
                return result;
            }

            epoll_event socketEvent;
            socketEvent.data.fd = this->_socket;
            socketEvent.data.ptr = this;
            socketEvent.events = EPOLLIN;

            if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, this->_socket, &socketEvent) == -1)
            {
                auto result = session_result(session_status::init_socket_setup_fail, errno);
                shutdown(this->_socket, SHUT_RDWR);
                close(this->_socket);
                this->_socket = -1;
                return result;
            }

            return session_result(session_status::init_success);
        };
        
        auto result = initSocket();

        if(result.status == session_status::init_success)
        {
            this->sendMessage(msg_headers::accept_connstart);
        }
        else
        {
            sendInitError();
        }

        return result;
    }

    session_result SessionUdp::proceed()
    {
        if (this->_socket == -1)
        {
            return session_result(session_status::proceed_udp_socket_was_closed);
        }
        
        itimerspec timerData;
        if (timerfd_gettime(this->timerfd, &timerData) == -1)
        {
            auto result = session_result(session_status::proceed_udp_timer_fail, errno);

            this->sendMessage(msg_headers::server_session_err);

            this->sessionClose();

            return result;
        }

        if (timerData.it_value.tv_sec == 0 && timerData.it_value.tv_nsec == 0)
        {
            this->isTimeout = true;
            return session_result(session_status::proceed_recv_timeout);
        }
        
        uint8_t msgBuff[MESSAGE_MAX_BUFFER];
        auto len = recv(this->_socket, msgBuff, MESSAGE_MAX_BUFFER, 0);

        if (len == -1 || static_cast<size_t>(len) < sizeof(msg_headers))
        {
            return session_result(session_status::proceed_recv_fail, errno);
        }
        if (static_cast<size_t>(len) < sizeof(msg_headers))
        {
            this->sendMessage(msg_headers::incorrect_msg);

            return session_result(session_status::init_udp_wrong_length);
        }
        else
        {
            auto header = *reinterpret_cast<msg_headers*>(msgBuff);

            if (!(static_cast<msg_headers_t>(header) & static_cast<msg_headers_t>(msg_headers::sender_client)))
            {
                this->sendMessage(msg_headers::incorrect_msg);
                return session_result(session_status::proceed_wrong_header);
            }
                
            if (len == sizeof(msg_headers))
            {
                session_result result;
                switch(header)
                {
                case msg_headers::client_connup:
                    {
                        this->timerReset();
                        if(this->sendMessage(msg_headers::server_connup) == -1)
                            result = session_result(session_status::proceed_send_fail, errno);
                        else
                            result = session_result(session_status::proceed_udp_connup);
                        break;
                    }
                        
                case msg_headers::client_conndown:
                    {
                        this->sessionClose();
                        result = session_result(session_status::proceed_disconnect);
                        break;
                    }

                case msg_headers::client_timeout:
                    {
                        this->sessionClose();
                        result = session_result(session_status::proceed_udp_client_timeout);
                        break;
                    }
                        
                default:
                    {
                        this->sendMessage(msg_headers::incorrect_msg);
                        result = session_result(session_status::proceed_wrong_header);
                        break;
                    }
                };

                return result;
                }
            else if (header == msg_headers::client_msg)
            {
                char buff[MESSAGE_MAX_BUFFER - sizeof(msg_headers)];
                
                auto sendLen = proceedMsg(reinterpret_cast<char*>(msgBuff + static_cast<ptrdiff_t>(sizeof(msg_headers))), len - sizeof(msg_headers), buff);
                
                if (this->sendMessage(msg_headers::server_msg, buff, sendLen) == -1)
                    return session_result(session_status::proceed_send_fail, errno);
                return session_result(session_status::proceed_msg_send);
            }
            else
            {
                this->sendMessage(msg_headers::incorrect_msg);
                return session_result(session_status::proceed_wrong_header);
            }
        }
        
        return session_result(session_status::proceed_unknown_fd);
    }

    bool SessionUdp::timerReset()
    {
        this->isTimeout = false;

        itimerspec timerspec;
        timerspec.it_interval.tv_sec = 0;
        timerspec.it_interval.tv_nsec = 0;
        timerspec.it_value.tv_sec = WAIT_TIMEOUT_SEC;
        timerspec.it_value.tv_nsec = 0;

        return timerfd_settime(this->timerfd, 0, &timerspec, nullptr) == 0;
    }
}
