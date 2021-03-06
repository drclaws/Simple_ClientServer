#include "MainConsole.hpp"

#include <iostream>

#include <sys/eventfd.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include <errno.h>
#include <unistd.h>
#include <limits>

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

        bool isExit = false;
        SessionClient * currentSession = nullptr;
        console_state currentState;
        
        socket_t sessionSocket = -1;

        auto switchState = [&currentState, &currentSession, &sessionSocket](console_state newState)
        {
            switch (newState)
            {
            case console_state::protocol_selection:
                if (currentSession != nullptr)
                {
                    delete currentSession;
                    sessionSocket = -1;
                    currentSession = nullptr;
                }
                std::cout << std::endl << "Select protocol: [u]dp or [t]cp" << std::endl;
                break;

            case console_state::address_input:
                std::cout << std::endl << "Input server IP-address" << std::endl;
                break;
            case console_state::connected:
                sessionSocket = currentSession->getSocket();
                std::cout << std::endl << "Connected. Input message" << std::endl;
                break;
            }

            std::cout << " >> " << std::flush;
            currentState = newState;
        };

        switchState(console_state::protocol_selection);

        while (!isExit)
        {
            fd_set fd_in;
            FD_ZERO(&fd_in);

            FD_SET(STDIN_FILENO, &fd_in);
            FD_SET(this->breakEventFd, &fd_in);

            int largerFd = this->breakEventFd > STDIN_FILENO ? this->breakEventFd : STDIN_FILENO;
            if (sessionSocket != -1)
            {
                FD_SET(sessionSocket, &fd_in);
                largerFd = sessionSocket > largerFd ? sessionSocket : largerFd;
            }
            
            timeval tv;
            
            tv.tv_sec = 5;
            tv.tv_usec = 0;

            int selectResult = select(largerFd + 1, &fd_in, 0, 0, &tv);

            if (selectResult == -1 && errno != EINTR)
            {
                std::cout << std::endl << "Select failed with code " << errno << std::endl;
                if (currentSession != nullptr)
                    delete currentSession;
                return -1;
            }

            if (FD_ISSET(this->breakEventFd, &fd_in))
            {
                eventfd_t decrement = 1;
                eventfd_read(this->breakEventFd, &decrement);

                std::cout << std::endl;

                if (currentState == console_state::protocol_selection)
                    isExit = true;
                else
                {
                    switchState(console_state::protocol_selection);
                }
                
                continue;
            }
            
            if (currentState == console_state::connected)
            {
                if (selectResult == 0)
                {
                    auto result = currentSession->sendConnup();
                    if (result.status != session_status::connup_up && result.status != session_status::connup_not_req)
                    {
                        if (result.status == session_status::connup_timeout)
                            std::cout << std::endl << "Server does not response";
                        else if (result.status == session_status::connup_timeout)
                            std::cout << std::endl << "Server closed session due to timeout";
                        else if (result.status == session_status::server_error)
                            std::cout << std::endl << "Server returned internal error";
                        else
                            std::cout << std::endl << "Error. Unknown status returned";
                        
                        if (result.err != 0)
                            std::cout << " with code " << result.err << std::endl << std::flush;
                        else
                            std::cout << std::endl << std::flush;

                        switchState(console_state::protocol_selection);

                        continue;
                    }
                }
                
                if (FD_ISSET(sessionSocket, &fd_in))
                {
                    auto result = currentSession->proceed();
                    
                    switch (result.status)
                    {
                    case session_status::proceed_msg_recv_fail:
                        std::cout << std::endl << "Message receiving failed";
                        break;
                    case session_status::proceed_disconnect:
                        std::cout << std::endl << "Session closed by server";
                        break;
                    case session_status::recv_wrong_length:
                        std::cout << std::endl << "Message with wrong length received";
                        break;
                    case session_status::proceed_server_error:
                        std::cout << std::endl << "Server error returned";
                        break;
                    case session_status::proceed_server_timeout:
                        std::cout << std::endl << "Received server timeout";
                        break;
                    default:
                        std::cout << std::endl << "Unknown error";
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

            if (FD_ISSET(STDIN_FILENO, &fd_in))
            {
                std::string line;
                std::getline(std::cin, line);

                int len = line.size();
                const char* linePtr = line.c_str();

                if (len == 0)
                {
                    std::cout << " >> " << std::flush;
                }
                else if (currentState == console_state::protocol_selection)
                {
                    if (len != 1 || (linePtr[0] != 'u' && linePtr[0] != 't'))
                    {
                        std::cout << "Invalid input. [u]dp or [t]cp?" << std::endl << 
                            " >> " << std::flush;
                    }
                    else
                    {
                        if (linePtr[0] == 't')
                            currentSession = new SessionTcp();
                        else
                            currentSession = new SessionUdp();
                        
                        switchState(console_state::address_input);
                    }
                }
                else if (currentState == console_state::address_input)
                {
                    auto address = inet_addr(linePtr);

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
                                std::cout << "Received message corrupted. Possible that client didn't receive session close message";
                                break;
                            case session_status::server_error:
                                std::cout << "Error on server-side";
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
                    auto result = currentSession->proceed(linePtr, len);
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

        if (currentSession != nullptr)
        {
            delete currentSession;
        }

        return 0;
    }
}
