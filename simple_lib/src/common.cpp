#include "../include/simple_lib/common.h"

#include <sys/epoll.h>

#if defined(O_NONBLOCK)
#include <fcntl.h>
#else
#include <sys/ioctl.h>
#endif

namespace simpleApp
{
    int set_nonblock(int fd)
    {
        int flags;
        
        #if defined(O_NONBLOCK)
        if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
            flags = 0;
        return fcntl(fd, F_SETFL, flags);
        
        #else
        flags = 1;
        return ioctl(fd, FIONBIO, &flags);
        
        #endif
    }
} // namespace simpleApp
