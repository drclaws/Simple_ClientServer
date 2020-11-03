#include "message.h"

#include <string>
#include <cstring>
#include <regex>

#include <simple_lib/common.h>

namespace simpleApp
{
    int proceedMsg(char* in, size_t len, char* out)
    {
        auto msgReceived = std::string(in, len);
        bool isFound = false;
        int value = 0;

        const std::regex r("(\\-|\\+)?[0123456789]+");

        for (std::sregex_iterator it = std::sregex_iterator(msgReceived.begin(), msgReceived.end(), r); 
                it != std::sregex_iterator(); it++)
        {
            isFound = true;
            std::smatch match;
            match = *it;
            value += std::stoi(match.str());
        }
        if (isFound)
        {
            auto line = std::to_string(value);
            std::memcpy(out, line.c_str(), line.size());
            return line.size();
        }
        else
        {
            std::memcpy(out, in, len);
            return len;
        }
    }
}
