#include "MainConsole.hpp"

#include <iostream>

#include <sys/eventfd.h>
#include <sys/epoll.h>

#include <errno.h>
#include <unistd.h>

#include "SessionClient.hpp"
#include "SessionTcp.hpp"
#include "SessionUdp.hpp"
#include "console_state.h"

namespace simpleApp
{
    MainConsole::MainConsole()
    {

    }

    MainConsole::~MainConsole()
    {
        if (this->breakEventFd != -1)
            close(this->breakEventFd);
    }

    int MainConsole::initBreak() 
    {
        if (this->breakEventFd != -1)
            close(this->breakEventFd);
        
        this->breakEventFd = eventfd(0, EFD_NONBLOCK);

        return this->breakEventFd == -1 ? errno : 0;
    }

    int MainConsole::raiseBreak()
    {
        if (this->breakEventFd != -1)
        {
            if (eventfd_write(this->breakEventFd, static_cast<eventfd_t>(1)) == -1)
                return errno;
        }

        return 0;
    }

    void cleanStdin()
    {
        // TODO
    }

    int getFromStdin(char* buffer)
    {
        // TODO
        return 0;
    }

    int MainConsole::consoleLoop()
    {
        int epollfd = epoll_create1(0);
        
        if (epollfd == -1)
        {
            std::cout << "EPoll creation failed with code " << errno << std::endl << std::flush;
            return -1;
        }

        epoll_event setupEvent;
        setupEvent.data.fd = this->breakEventFd;
        setupEvent.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, this->breakEventFd, &setupEvent) == -1)
        {
            std::cout << "EPOLL_CTL_ADD of break event fd failed with code " << errno << std::endl << std::flush;
            close(epollfd);
            return -1;
        }
        
        setupEvent.data.fd = STDIN_FILENO;
        setupEvent.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &setupEvent) == -1)
        {
            std::cout << "EPOLL_CTL_ADD of break event fd failed with code " << errno << std::endl << std::flush;
            epoll_ctl(epollfd, EPOLL_CTL_DEL, this->breakEventFd, nullptr);
            close(epollfd);
            return -1;
        }

        bool isExit = false;
        console_state currentState = console_state::protocol_selection;
        SessionClient* currentSession = nullptr;

        std::cout << "Select protocol: [u]dp or [t]cp - or press Ctrl+C for exit" << std::endl << std::flush;
        std::cout << " >> " << std::flush;

        while (!isExit)
        {
            const size_t eventsSize = 4;
            epoll_event events[eventsSize];

            console_state nextState = console_state::none;

            int len = epoll_wait(epollfd, events, eventsSize, -1);
            for(size_t i = 0; static_cast<int>(i) < len; i++)
            {
                if (events[i].data.fd == this->breakEventFd)
                {
                    switch (currentState)
                    {
                    case console_state::protocol_selection:
                        nextState = console_state::none;
                        isExit = true;
                        break;
                    
                    case console_state::msg_input:
                    case console_state::wait_result:
                    case console_state::connect_wait:
                        delete currentSession;
                        currentSession = nullptr;
                    case console_state::address_input:
                        nextState = console_state::protocol_selection;
                        break;
                    default:
                        break;
                    }
                    
                    cleanStdin();

                    break;
                }

                // Non-console event and have session
                if (events[i].data.fd != STDIN_FILENO && currentSession != nullptr)
                {
                    //cleanStdin();
                    
                    switch (currentState)
                    {
                    case console_state::msg_input:
                        /* code */
                        break;
                    
                    default:
                        break;
                    }
                    // TODO
                }

                // Console input
                if (events[i].data.fd == STDIN_FILENO)
                {
                    char buffer[MESSAGE_MAX_BUFFER - sizeof(msg_headers)];
                    auto len = getFromStdin(buffer);
                    
                    if (len < 1)
                    {
                        cleanStdin();
                        break;
                    }

                    switch (currentState)
                    {
                    case console_state::protocol_selection:
                        if (buffer[0] == 't')
                        {
                            currentSession = new SessionTcp(epollfd);
                            nextState = console_state::address_input;
                        }
                        else if (buffer[0] == 'u')
                        {
                            currentSession = new SessionUdp(epollfd);
                            nextState = console_state::address_input;
                        }
                        else
                        {
                            std::cout << "Incorrect choice" << std::endl << std::flush;
                        }
                        break;
                    
                    case console_state::address_input:
                        {
                            auto result = currentSession->init(std::string(buffer, len));
                            switch (result.status)
                            {
                            case session_status::init_success:
                                nextState = console_state::msg_input;
                                break;
                            
                            case session_status::init_udp_conn_wait:
                                nextState = console_state::connect_wait;
                                break;
                            
                            default:
                                std::cout << "Connection error" << std::endl << std::flush;
                                delete currentSession;
                                currentSession = nullptr;
                                nextState = console_state::protocol_selection;
                                break;
                            }
                            break;
                        }
                    case console_state::msg_input:
                        {
                            auto result = currentSession->proceed(events[i].data.fd, buffer, len);
                            switch (result.status)
                            {
                            case session_status::proceed_msg_send:
                                
                                break;
                            
                            default:
                                break;
                            }
                        }
                    default:
                        break;
                    }

                    cleanStdin();
                    break;
                }
            }
            
            // Switch state
            switch (nextState)
            {
            case console_state::protocol_selection:
                {
                    std::cout << "Select protocol: [u]dp or [t]cp - or press Ctrl+C for exit" << std::endl << std::flush;
                    break;
                }
            
            case console_state::address_input:
                {
                    std::cout << "Input IP address of server. To change protocol press Ctrl+C" << std::endl << std::flush;
                    break;
                }

            case console_state::msg_input:
                {
                    std::cout << "Connected to server. For disconnect press Ctrl+C" << std::endl << std::flush;
                    break;
                }
            default:
                break;
            }

            if (nextState != console_state::none)
                currentState = nextState;

            switch (currentState)
            {
            case console_state::protocol_selection:
            case console_state::address_input:
            case console_state::msg_input:
                std::cout << " >> " << std::endl;
                break;
            default:
                break;
            }
        }

        if (currentSession != nullptr)
            delete currentSession;

        epoll_ctl(epollfd, EPOLL_CTL_DEL, this->breakEventFd, 0);
        close(epollfd);

        return 0;
    }
}
