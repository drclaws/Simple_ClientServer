#include "MainConsole.hpp"

#include <sys/eventfd.h>

#include <errno.h>
#include <unistd.h>

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
        return 0;
    }
}
