#include "MainConsole.hpp"

#include <iostream>

#include <sys/eventfd.h>
#include <sys/select.h>
#include <arpa/inet.h>

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

    int MainConsole::consoleLoop()
    {
        if (this->breakEventFd == -1)
        {
            std::cout << "Break event is not set" << std::endl;
            return -1;
        }
        
        timeval tv;
        
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        bool isExit = false;
        SessionClient * currentSession = nullptr;
        console_state currentState;

        auto switchState = [&currentState, &currentSession, &isExit](console_state newState)
        {
            // TODO
        };

        switchState(console_state::protocol_selection);

        while (!isExit)
        {
            fd_set fd_in;
            FD_ZERO(&fd_in);

            FD_SET(this->breakEventFd, &fd_in);
            FD_SET(STDIN_FILENO, &fd_in);

            int largerFd = this->breakEventFd > STDIN_FILENO ? this->breakEventFd : STDIN_FILENO;
            if (currentSession != nullptr)
            {
                auto sessionSocket = currentSession->getSocket();
                largerFd = sessionSocket > largerFd ? sessionSocket : largerFd;
            }
                
            int selectResult = select(largerFd + 1, &fd_in, 0, 0, &tv);

            if (selectResult == -1)
            {
                std::cout << "Select failed with code " << errno;
                if (currentSession != nullptr)
                    delete currentSession;
                return -1;
            }

            if (currentSession != nullptr)
            {
                if (selectResult == 0)
                {
                    auto result = currentSession->sendConnup();
                    if (result.status != session_status::connup_up && result.status != session_status::connup_not_req)
                    {
                        if (result.status == session_status::connup_timeout)
                            std::cout << std::endl << "Server does not response";
                        else
                            std::cout << std::endl << "Unknown error";
                        
                        if (result.err != 0)
                            std::cout << " with code " << result.err << std::endl << std::flush;
                        else
                            std::cout << std::endl << std::flush;

                        switchState(console_state::address_input);

                        continue;
                    }
                }
                
                // Something came to socket without request
                if (FD_ISSET(currentSession->getSocket(), &fd_in))
                {
                    auto result = currentSession->proceed();
                    
                    switch (result.status)
                    {
                    case session_status::proceed_msg_recv_fail:
                        std::cout << "Message receiving failed";
                        break;
                    case session_status::proceed_disconnect:
                        std::cout << "Session shutdown by server";
                        break;
                    case session_status::recv_wrong_length:
                        std::cout << "Message with wrong length received";
                        break;
                    case session_status::proceed_server_error:
                        std::cout << "Server error returned";
                        break;
                    case session_status::proceed_server_timeout:
                        std::cout << "Received server timeout";
                        break;
                    default:
                        std::cout << "Unknown error";
                        break;
                    }
                     
                    if (result.err != 0)
                        std::cout << " with code " << result.err << std::endl << std::flush;
                    else
                        std::cout << std::endl << std::flush;
                    
                    switchState(console_state::protocol_selection);

                    continue;
                }
            }
            else if (selectResult == 0)
            {
                continue;
            }

            if (FD_ISSET(this->breakEventFd, &fd_in))
            {
                if (currentState == console_state::protocol_selection)
                    isExit = true;
                else
                    switchState(console_state::protocol_selection);
                
                continue;
            }

            if (FD_ISSET(STDIN_FILENO, &fd_in))
            {
                char buffer[MESSAGE_MAX_BUFFER - sizeof(msg_headers)];

                int len = 0;
                // TODO read from stdin
                // len = read(stdin, buffer);
                    
                if (len == 0)
                {
                    std::cout << " >> " << std::flush;
                }
                else if (currentState == console_state::protocol_selection)
                {
                    if (len != 1 || (buffer[0] != 'u' && buffer[0] != 't'))
                    {
                        std::cout << "Invalid input. [u]dp or [t]cp?" << std::endl << 
                            " >> " << std::flush;
                    }
                    else
                    {
                        if (buffer[0] == 'u')
                            currentSession = new SessionTcp();
                        else
                            currentSession = new SessionUdp();
                        
                        switchState(console_state::address_input);
                    }
                }
                else if (currentState == console_state::address_input)
                {
                    auto address = inet_addr(buffer);

                    if (address == INADDR_NONE)
                    {
                        std::cout << "The entered IP address is incorrect. Try again" << std::endl
                            << " >> " << std::flush; 
                    }
                    else
                    {
                        auto result = currentSession->init(address);
                        if (result.status == session_status::init_success)
                        {
                            switchState(console_state::connected);
                        }
                        else
                        {
                            switch(result.status)
                            {
                            case session_status::init_socket_fail:
                                std::cout << "Socket initialization failed";
                                break;
                            case session_status::init_connection_fail:
                                std::cout << "Connection failed";
                                break;
                            case session_status::init_udp_send_up_fail:
                            case session_status::init_udp_recv_up_fail:
                                std::cout << "Connection request failed";
                                break;
                            case session_status::recv_wrong_length:
                            case session_status::recv_wrong_header:
                                std::cout << "Received message corrupted";
                                break;
                            default:
                                std::cout << "Unknown error";
                                break;
                            }

                            if (result.err != 0)
                                std::cout << " with code " << result.err << std::endl << std::flush;
                            else
                                std::cout << std::endl << std::flush;
                            
                            switchState(console_state::protocol_selection);
                        }
                    }
                }
                else if (currentState == console_state::connected)
                {
                    auto result = currentSession->proceed(buffer, len);
                    if (result.status == session_status::proceed_msg)
                    {
                        std::cout << "Received answer: '" << result.recvMsg << "'" << std::endl <<
                            " >> " << std::flush;
                    }
                    else
                    {
                        switch(result.status)
                        {
                        case session_status::proceed_msg_send_fail:
                            std::cout << "Message sending failed";
                            break;
                        case session_status::proceed_msg_recv_fail:
                            std::cout << "Message receiving failed";
                            break;
                        case session_status::recv_wrong_length:
                        case session_status::recv_wrong_header:
                            std::cout << "Message has been corrupted";
                            break;
                        default:
                            std::cout << "Unknown error";
                            break;
                        }

                        if (result.err != 0)
                            std::cout << " with code " << result.err << std::endl << std::flush;
                        else
                            std::cout << std::endl << std::flush;
                        
                        switchState(console_state::protocol_selection);
                    }
                }                
            }
        }

        return 0;
    }
}
