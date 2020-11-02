#include "Server.hpp"

#include <iostream>
#include <set>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <strings.h>

#include <unistd.h>

#include <simple_lib/common.h>

#include "SessionServer.hpp"
#include "SessionUdp.hpp"
#include "SessionTcp.hpp"

namespace simpleApp
{
    int modeEpoll(int epollfd, int epollop, int fd, void* ptr, uint32_t events)
    {
        if (events == 0)
        {
            return epoll_ctl(epollfd, epollop, fd, 0);
        }
        else
        {
            struct epoll_event stopEvent;
            stopEvent.data.ptr = ptr;
            stopEvent.events = events;

            return epoll_ctl(epollfd, epollop, fd, &stopEvent);
        }
    }

    inline int addToEPoll(int epollfd, int fd, void* ptr, uint32_t events)
    {
        return modeEpoll(epollfd, EPOLL_CTL_ADD, fd, ptr, events);
    }

    inline int removeFromEPoll(int epollfd, int fd)
    {
        return modeEpoll(epollfd, EPOLL_CTL_DEL, fd, 0, 0);
    }

    inline socket_t createTcpSocket(int& err)
    {
        socket_t newSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (newSocket == -1)
        {
            err = errno;
            return -1;
        }
        else if (set_nonblock(newSocket) == -1)
        {
            err = errno;
            close(newSocket);
            return -1;
        }
        
        int optval = 1;
        if(setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        {
            err = errno;
            close(newSocket);
            return -1;
        }

        sockaddr_in serverTcpAddress;
        bzero(&serverTcpAddress, sizeof(serverTcpAddress));
        serverTcpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverTcpAddress.sin_port = htons(PUBLIC_PORT);
        serverTcpAddress.sin_family = AF_INET;
        
        if (bind(newSocket, (sockaddr *)&serverTcpAddress, sizeof(serverTcpAddress)) == -1)
        {
            err = errno;
            close(newSocket);
            return -1;
        }

        if (listen(newSocket, SOMAXCONN) == -1)
        {
            err = errno;
            shutdown(newSocket, SHUT_RDWR);
            close(newSocket);
            return -1;
        }

        err = 0;
        return newSocket;
    }

    inline socket_t createUdpSocket(int& err)
    {
        socket_t newSocket = socket(AF_INET, SOCK_DGRAM, 0);

        if (newSocket == -1)
        {
            err = errno;
            return -1;
        }
        else if (set_nonblock(newSocket) == -1)
        {
            err = errno;
            close(newSocket);
            return -1;
        }

        int optval = 1;
        if(setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        {
            err = errno;
            close(newSocket);
            return -1;
        }

        sockaddr_in serverUdpAddress;
        bzero(&serverUdpAddress, sizeof(serverUdpAddress));
        serverUdpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverUdpAddress.sin_port = htons(PUBLIC_PORT);
        serverUdpAddress.sin_family = AF_INET;

        if (bind(newSocket, (sockaddr *)&serverUdpAddress, sizeof(serverUdpAddress)) == -1)
        {
            err = errno;
            close(newSocket);
            return -1;
        }

        err = 0;
        return newSocket;
    }

    Server::Server()
    {
        
    }
    
    Server::~Server()
    {
        if (this->stopEventFd != -1)
            close(this->stopEventFd);
    }

    int Server::serverLoop()
    {
        if(this->stopEventFd == -1)
        {
            std::cout << "Stop event object is not initialized" << std::endl << std::flush;
            return -1;
        }

        std::cout << "Starting server" << std::endl << std::flush;

        int err;
        socket_t masterTcpSocket = createTcpSocket(err);

        if (masterTcpSocket == -1)
            std::cout << "TCP master socket initialization failed with code " << err << std::endl << std::flush;
        
        socket_t masterUdpSocket = createUdpSocket(err);

        if (masterUdpSocket == -1)
            std::cout << "UDP master socket initialization failed with code " << err << std::endl << std::flush;

        if (masterTcpSocket == -1 && masterUdpSocket == -1)
        {
            std::cout << "Every master sockets initialization failed" << std::endl << std::flush;
            return -1;
        }

        int epollfd = epoll_create1(0);
        bool isFailed = false;

        if (epollfd == -1)
        {
            std::cout << "EPoll creation failed with code " << errno << std::endl << std::flush;
            isFailed = true;
        }
        else if(addToEPoll(epollfd, this->stopEventFd, &this->stopEventFd, EPOLLIN) == -1)
        {
            std::cout << "EPOLL_CTL_ADD of stop event fd failed with code " << errno << std::endl << std::flush;
            isFailed = true;
        }
        else
        {    
            if (masterTcpSocket != -1)
            {
                if (addToEPoll(epollfd, masterTcpSocket, &masterTcpSocket, EPOLLIN | EPOLLET) == -1)
                {
                    std::cout << "EPOLL_CTL_ADD of TCP master socket failed with code " << errno << std::endl << std::flush;
                    shutdown(masterTcpSocket, SHUT_RDWR);
                    close(masterTcpSocket);
                    masterTcpSocket = -1;
                }
                else 
                    std::cout << "TCP-connection enabled" << std::endl;
            }

            if (masterUdpSocket != -1)
            {
                if (addToEPoll(epollfd, masterUdpSocket, &masterUdpSocket, EPOLLIN | EPOLLET) == -1)
                {
                    std::cout << "EPOLL_CTL_ADD of UDP master socket failed with code " << errno << std::endl << std::flush;
                    shutdown(masterUdpSocket, SHUT_RDWR);
                    close(masterUdpSocket);
                    masterUdpSocket = -1;
                }
                else
                    std::cout << "UDP-connection enabled" << std::endl;
            }

            if (masterTcpSocket == -1 && masterUdpSocket == -1) 
            {
                std::cout << "There are no master sockets linked to epoll object" << std::endl << std::flush;
                isFailed = true;
            }
            else
            {  
                const size_t MAX_EVENTS_BUFFER = 10000;
                epoll_event * events = static_cast<epoll_event *>(calloc(MAX_EVENTS_BUFFER, sizeof(epoll_event)));
                
                std::set<SessionServer*> slaveSocketsMap = std::set<SessionServer *>();

                bool stopEventHappened = false;

                auto initSession = [&slaveSocketsMap, &masterTcpSocket, &masterUdpSocket, &epollfd](epoll_event& event)
                {
                    SessionServer* clientSession;
                    session_result result;

                    if (event.data.ptr == &masterTcpSocket)
                    {
                        clientSession = new SessionTcp(epollfd);
                        result = clientSession->init(masterTcpSocket);
                    }
                    else if (event.data.ptr == &masterUdpSocket)
                    {
                        clientSession = new SessionUdp(epollfd);
                        result = clientSession->init(masterUdpSocket);
                    }
                    else
                        return false;
                    
                    std::string msg = clientSession->getName() + ". ";
                    
                    bool toInsert = false;
                    switch (result.status)
                    {
                    case session_status::init_success:
                        {
                            toInsert = true;
                            msg += "Session initialized";
                            break;
                        }
                        
                    case session_status::init_socket_setup_fail:
                        {
                            msg += "Socket initialization failed";
                            break;
                        }
                        
                    case session_status::init_listener_fail:
                        {
                            msg += "Message receiving from listener failed";
                            break;
                        }
                        
                    case session_status::init_udp_wrong_header:
                        {
                            msg += "Header check failed";
                            break;
                        }
                        
                    case session_status::init_udp_wrong_length:
                        {
                            msg += "Message length check failed";
                            break;
                        }
                        
                    case session_status::init_udp_timer_fail:
                        {
                            msg += "Timeout timer setup failed";
                            break;
                        }

                    default:
                        {
                            msg += "Unknown status returned";
                            break;
                        }

                    }
                    
                    if (result.err != 0)
                        msg += std::string(" with code ") + std::to_string(result.err);

                    if (toInsert)
                        slaveSocketsMap.insert(clientSession);
                    else
                        delete clientSession;

                    std::cout << msg << std::endl;

                    return true;
                };
                while (!stopEventHappened)
                {
                    std::set<SessionServer*> slavesForRemove = std::set<SessionServer*>();
                    int N = epoll_wait(epollfd, events, MAX_EVENTS_BUFFER, -1);
                    for (size_t i = 0; static_cast<int>(i) < N; i++)
                    {
                        if (events[i].data.ptr == &this->stopEventFd)
                        {
                            eventfd_t decrement = 1;
                            
                            std::string msg = "Stop event happened";

                            if (eventfd_read(this->stopEventFd, static_cast<eventfd_t *>(&decrement)) == -1)
                            {
                                msg += std::string("\nStop event decrementation failed with code ") +
                                    std::to_string(errno) + ". Server will stop anyway";
                            }
                            if (masterTcpSocket != -1)
                            {
                                removeFromEPoll(epollfd, masterTcpSocket);
                            }
                            if (masterUdpSocket != -1)
                            {
                                removeFromEPoll(epollfd, masterUdpSocket);
                            }
                            
                            std::cout << msg << std::endl << std::flush;

                            stopEventHappened = true;
                        }
                        else if (initSession(events[i]))
                        {
                            continue;
                        }
                        else
                        {
                            auto clientSession = reinterpret_cast<SessionServer*>(events[i].data.ptr);
                            if(slaveSocketsMap.find(clientSession) == slaveSocketsMap.end())
                            {
                                std::cout << "Unknown event happened" << std::endl;
                            }
                            else
                            {
                                auto result = clientSession->proceed();
                                std::string msg = "";

                                switch (result.status)
                                {
                                case session_status::proceed_msg_send:
                                case session_status::proceed_udp_connup:
                                    {
                                        slavesForRemove.erase(clientSession);
                                        break;
                                    }

                                case session_status::proceed_disconnect:
                                    {
                                        slavesForRemove.insert(clientSession);
                                        msg = std::string("Session has been closed by client");
                                        break;
                                    }

                                case session_status::proceed_udp_timeout:
                                    {
                                        slavesForRemove.insert(clientSession);
                                        msg = std::string("Timeout triggered");
                                        break;
                                    }

                                case session_status::proceed_udp_client_timeout:
                                    {
                                        slavesForRemove.insert(clientSession);
                                        msg = std::string("Session has been closed by client because of timeout");
                                        break;
                                    }

                                case session_status::proceed_udp_timer_fail:
                                    {
                                        slavesForRemove.insert(clientSession);
                                        msg = std::string("Session has been closed because of timer failure");
                                        break;
                                    }

                                case session_status::proceed_recv_fail:
                                    {
                                        msg = std::string("RECV failed");
                                        break;
                                    }

                                case session_status::proceed_send_fail:
                                    {
                                        msg = std::string("SEND failed");
                                        break;
                                    }

                                case session_status::proceed_wrong_header:
                                    {
                                        msg = std::string("Received message with wrong header");
                                        break;
                                    }

                                case session_status::proceed_unknown_fd:
                                    {
                                        msg = std::string("Unknown fd connected");
                                        break;
                                    }

                                case session_status::proceed_tcp_wrong_size:
                                    {
                                        msg = std::string("Receive message has wrong size");
                                        break;
                                    }

                                case session_status::proceed_udp_false_timeout:
                                case session_status::proceed_udp_socket_was_closed:
                                    break;
                                
                                default:
                                    {
                                        msg = std::string("Unknown error");
                                        break;
                                    }
                                    
                                }
                                
                                if (result.err != 0)
                                    msg += std::string(" Returned code: ") + std::to_string(result.err);
                                
                                if (msg != std::string(""))
                                {
                                    std::cout << clientSession->getName() << ". " << msg << std::endl;
                                }
                            }
                        }
                    }

                    for(auto& val: slavesForRemove)
                    {
                        slaveSocketsMap.erase(val);
                        delete val;
                    }
                }

                std::cout << "Closing all sessions" << std::endl;

                free(events);
                for(auto& val: slaveSocketsMap)
                    delete val;
            }
            
            removeFromEPoll(epollfd, this->stopEventFd);
        }
        
        if(epollfd != -1)
            close(epollfd);
        if (masterTcpSocket != -1)
        {
            shutdown(masterTcpSocket, SHUT_RDWR);
            close(masterTcpSocket);
        }
        if (masterUdpSocket != -1)
        {
            shutdown(masterTcpSocket, SHUT_RDWR);
            close(masterUdpSocket);
        }

        return isFailed ? -1 : 0;
    }

    int Server::initStop() 
    {
        if (this->stopEventFd != -1)
            close(this->stopEventFd);
        
        this->stopEventFd = eventfd(0, EFD_NONBLOCK);

        return this->stopEventFd == -1 ? errno : 0;
    }

    int Server::raiseStop()
    {
        if (this->stopEventFd != -1)
        {
            if (eventfd_write(this->stopEventFd, static_cast<eventfd_t>(1)) == -1)
                return errno;
        }

        return 0;
    }
} // namespace simpleApp
