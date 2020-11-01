#pragma once

namespace simpleApp
{
    class MainConsole
    {
    public:
        MainConsole();
        ~MainConsole();

        int initBreak();
        int raiseBreak();
        int consoleLoop();
    private:
        int breakEventFd = -1;
    };
}
