#include "SessionTcp.hpp"

#include <sys/socket.h>

#include <unistd.h>

namespace simpleApp
{
    SessionTcp::SessionTcp() : SessionClient()
    {

    } 

    SessionTcp::~SessionTcp()
    {

    }

    session_result SessionTcp::init(in_addr_t address)
    {
        return this->connectSocket(address, SOCK_STREAM);
    }
} 