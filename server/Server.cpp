#include <iostream>
#include <set>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <unistd.h>

#include <simple_lib/common.h>

#include "Server.hpp"
#include "ClientSession.hpp"
#include "UdpSession.hpp"
#include "TcpSession.hpp"

namespace simpleApp
{
    int modeEpoll(int& epollfd, int epollop, int& fd, uint32_t events)
    {
        if (events == 0)
        {
            return epoll_ctl(epollfd, epollop, fd, 0);
        }
        else
        {
            struct epoll_event stopEvent;
            stopEvent.data.fd = fd;
            stopEvent.events = events;

            return epoll_ctl(epollfd, epollop, fd, &stopEvent);
        }
    }

    inline int addToEPoll(int& epollfd, int& fd, uint32_t events)
    {
        return modeEpoll(epollfd, EPOLL_CTL_ADD, fd, events);
    }

    inline int removeFromEPoll(int& epollfd, int& fd)
    {
        return modeEpoll(epollfd, EPOLL_CTL_DEL, fd, 0);
    }

    inline socket_t createTcpSocket(uint16_t& port, int& err)
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

        struct sockaddr_in serverTcpAddress;
        serverTcpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverTcpAddress.sin_port = htons(port);
        serverTcpAddress.sin_family = AF_INET;
        
        if (bind(newSocket, (struct sockaddr *)&serverTcpAddress, sizeof(serverTcpAddress)) == -1)
        {
            err = errno;
            close(newSocket);
            return -1;
        }

        err = 0;
        return newSocket;
    }

    inline socket_t createUdpSocket(uint16_t& port, int& err)
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

        struct sockaddr_in serverUdpAddress;
        serverUdpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverUdpAddress.sin_port = htons(port);
        serverUdpAddress.sin_family = AF_INET;

        if (bind(newSocket, (struct sockaddr *)&serverUdpAddress, sizeof(serverUdpAddress)) == -1)
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
        if (this->stopObject != -1)
            close(this->stopObject);
    }

    int Server::serverLoop(uint16_t port)
    {
        if(this->stopObject == -1)
        {
            std::cout << "Stop event object is not initialized" << std::endl;
            return -1;
        }

        std::cout << "Starting server" << std::endl;

        int err;
        socket_t masterTcpSocket = createTcpSocket(port, err);

        if (masterTcpSocket == -1)
            std::cout << "TCP master socket initialization failed with code " << err << std::endl;
        
        socket_t masterUdpSocket = createUdpSocket(port, err);

        if (masterUdpSocket == -1)
            std::cout << "UDP master socket initialization failed with code " << err << std::endl;

        if (masterTcpSocket == -1 && masterUdpSocket == -1)
        {
            std::cout << "Every master sockets initialization failed" << std::endl;
            return -1;
        }

        int epollfd = epoll_create1(0);
        bool isFailed = false;

        if (epollfd == -1)
        {
            std::cout << "EPoll creation failed with code " << errno << std::endl;
            isFailed = true;
        }
        else if(addToEPoll(epollfd, this->stopObject, EPOLLIN))
        {
            std::cout << "EPOLL_CTL_ADD of stop event object failed with code " << errno << std::endl;
            isFailed = true;
        }
        else
        {            
            if (masterTcpSocket != -1 && addToEPoll(epollfd, masterTcpSocket, EPOLLIN | EPOLLET) == -1)
            {
                std::cout << "EPOLL_CTL_ADD of TCP master socket failed with code " << errno << std::endl;
                shutdown(masterTcpSocket, SHUT_RDWR);
                close(masterTcpSocket);
                masterTcpSocket = -1;
            }

            if (masterUdpSocket != -1 && addToEPoll(epollfd, masterUdpSocket, EPOLLIN | EPOLLET) == -1)
            {
                std::cout << "EPOLL_CTL_ADD of UDP master socket failed with code " << errno << std::endl;
                shutdown(masterUdpSocket, SHUT_RDWR);
                close(masterUdpSocket);
                masterUdpSocket = -1;
            }
            
            if (masterTcpSocket == -1 && masterUdpSocket == -1) 
            {
                std::cout << "There are no master sockets linked to epoll object" << std::endl;
                isFailed = true;
            }
            else
            {  
                const size_t MAX_EVENTS_BUFFER = 10000;
                struct epoll_event * events = static_cast<struct epoll_event *>(calloc(MAX_EVENTS_BUFFER, sizeof(struct epoll_event)));
                
                std::set<ClientSession*> slaveSocketsMap = std::set<ClientSession *>();

                bool stopEventHappened = false;

                auto initSession = [&slaveSocketsMap, masterTcpSocket, masterUdpSocket, &epollfd, port](struct epoll_event& event)
                {
                    ClientSession* clientSession;
                    session_result result;

                    if (event.data.fd == masterTcpSocket)
                    {
                        clientSession = new TcpSession(epollfd);
                        result = clientSession->init(masterTcpSocket, port);
                    }
                    else if (event.data.fd == masterUdpSocket)
                    {
                        clientSession = new UdpSession(epollfd);
                        result = clientSession->init(masterUdpSocket, port);
                    }
                    else
                        return false;
                    
                    
                    std::cout << (event.data.fd == masterTcpSocket ? "TCP" : "UDP");

                    auto address = clientSession->getAddress();
                    
                    if(address != nullptr)
                    {
                        uint32_t addressConverted = ntohl(address->sin_addr.s_addr);
                        uint16_t portConverted = ntohs(address->sin_port);

                        uint8_t* addressByBytes = reinterpret_cast<uint8_t*>(&addressConverted);
                        std::cout << "(" << static_cast<int>(addressByBytes[0]) << "." <<
                            static_cast<int>(addressByBytes[1]) << "." <<
                            static_cast<int>(addressByBytes[2]) << "." <<
                            static_cast<int>(addressByBytes[3]) << "). ";
                    }
                    else
                        std::cout << ". ";
                    
                    bool toInsert = false;
                    switch (result.status)
                    {
                    case session_status::init_success:
                        toInsert = true;
                        slaveSocketsMap.insert(clientSession);
                        break;
                    case session_status::init_socket_setup_fail:
                        std::cout << "Socket initialization failed";
                        break;
                    case session_status::init_udp_listener_fail:
                        std::cout << "Message receiving from listener failed";
                        break;
                    case session_status::init_udp_wrong_header:
                        std::cout << "Header check failed";
                        break;
                    case session_status::init_udp_wrong_length:
                        std::cout << "Message length check failed";
                        break;
                    case session_status::init_udp_timer_fail:
                        std::cout << "Timeout timer setup failed";
                        break;
                    default:
                        std::cout << "Unknown status returned";
                        break;
                    }
                    
                    if (result.err != 0)
                        std::cout << " with code " << result.err << std::endl;
                    else 
                        std::cout << std::endl;

                    if (toInsert)
                        slaveSocketsMap.insert(clientSession);
                    else
                        delete clientSession;

                    return true;
                };
                while (!stopEventHappened)
                {
                    std::set<ClientSession*> slavesForRemove = std::set<ClientSession*>();
                    int N = epoll_wait(epollfd, events, MAX_EVENTS_BUFFER, -1);
                    for (size_t i = 0; i < N; i++)
                    {
                        if (events[i].data.fd == this->stopObject)
                        {
                            eventfd_t decrement = 1;
                            std::cout << "Stop event happened" << std::endl;
                            if (eventfd_read(this->stopObject, static_cast<eventfd_t *>(&decrement)) == -1)
                            {
                                std::cout << "Stop event decrementation failed with code " << errno << std::endl <<
                                    "Server will stop anyway" << std::endl;
                            }
                            if (masterTcpSocket != -1)
                            {
                                removeFromEPoll(epollfd, masterTcpSocket);
                            }
                            if (masterUdpSocket != -1)
                            {
                                removeFromEPoll(epollfd, masterUdpSocket);
                            }
                            
                            stopEventHappened = true;
                        }
                        else if (initSession(events[i]))
                        {
                            continue;
                        }
                        else if (events[i].data.ptr != nullptr)
                        {
                            continue;
                            /*
                            auto clientSession = reinterpret_cast<ClientSession*>(events[i].data.ptr);
                            if (clientSession->proceed(events[i]) == -1)
                            {
                                slavesForRemove.push_back(clientSession);
                            }*/
                        }
                        else
                        {
                            std::wcout << L"Found unknown decriptor. Trying to remove from epoll" << std::endl;
                            if (removeFromEPoll(epollfd, events[i].data.fd) == -1)
                                std::wcout << L"Unknown decriptor removal failed with code " << errno << std::endl;
                            removeFromEPoll(epollfd, events[i].data.fd);
                        }
                    }

                    for(auto& val: slavesForRemove)
                    {
                        slaveSocketsMap.erase(val);
                        delete val;
                    }
                }

                free(events);
                for(auto& val: slaveSocketsMap)
                    delete val;
            }
            
            removeFromEPoll(epollfd, this->stopObject);
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
        if (this->stopObject != -1)
            close(this->stopObject);
        
        this->stopObject = eventfd(0, EFD_NONBLOCK);

        return this->stopObject == -1 ? errno : 0;
    }

    int Server::stop()
    {
        if (this->stopObject != -1)
        {
            if (eventfd_write(this->stopObject, static_cast<eventfd_t>(1)) == -1)
                return errno;
        }

        return 0;
    }
} // namespace simpleApp
