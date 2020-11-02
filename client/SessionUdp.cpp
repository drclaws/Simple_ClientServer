#include "SessionUdp.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>

#include <unistd.h>

#include <simple_lib/common.h>

namespace simpleApp
{
    SessionUdp::SessionUdp(int epollfd) : SessionClient(epollfd)
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
                const msg_headers headerBuff = msg_headers::client_timeout;
                send(this->_socket, &headerBuff, sizeof(headerBuff), 0);
            }
            else 
            {
                const msg_headers headerBuff = msg_headers::client_conndown;
                send(this->_socket, &headerBuff, sizeof(headerBuff), 0);
            }
            
        }
    }

    session_result SessionUdp::init(std::string address)
    {
        this->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (this->timerfd == -1)
        {
            return session_result(session_status::init_udp_timer_fail, errno);
        }

        epoll_event timerEvent;
        timerEvent.data.fd = this->timerfd;
        timerEvent.events = EPOLLET | EPOLLIN;

        if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, this->timerfd, &timerEvent) == -1)
        {
            auto result = session_result(session_status::init_udp_timer_fail, errno);
            close(this->timerfd);
            this->timerfd = -1;
            return result;
        }

        auto baseInitResult = this->connectSocket(address, SOCK_DGRAM);

        if (baseInitResult.status != session_status::init_success)
            return baseInitResult;

        const msg_headers buffHeader = msg_headers::client_connup;
        if (send(this->_socket, &buffHeader, sizeof(buffHeader), 0) == -1)
        {
            auto result = session_result(session_status::init_udp_send_up_fail, errno);
            this->sessionClose();
            return result;
        }

        if (this->timerSet(timer_status::connectWait) == -1)
        {
            auto result = session_result(session_status::udp_timer_set_fail, errno);
            this->sessionClose();
            return result;
        }

        return session_result(session_status::init_udp_conn_wait);
    }

    session_result SessionUdp::proceed(int fd, char* sendMsg, size_t msgSize)
    {
        if (fd == this->timerfd)
        {
            
        }
        else if (fd == this->_socket)
        {

        }
        return session_result();
    }

    int SessionUdp::timerSet(timer_status status)
    {
        itimerspec tv;
        tv.it_value.tv_nsec = 0;
        tv.it_interval.tv_sec = 0;
        tv.it_interval.tv_nsec = 0;

        switch(status)
        {
        case timer_status::idle:
            tv.it_value.tv_sec = 5;
            break;
        case timer_status::connectWait:
        case timer_status::dataWait:
            tv.it_value.tv_sec = WAIT_TIMEOUT_SEC;
            break;
        default:
            return -1;
        }
        
        this->timerStatus = status;
        return timerfd_settime(this->timerfd, 0, &tv, nullptr);
    }
}