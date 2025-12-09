#ifndef FLASKCPP_UTILS_LOG
#define FLASKCPP_UTILS_LOG

#include <iostream>
#include <functional>

namespace flaskcpp
{

struct LogMsg
{
    int level;
    std::string content;
    int line;
    std::string file;
    std::string function;
};

}

// level, msg, line, file, function
using loggerWrite = std::function<void(flaskcpp::LogMsg)>;



#endif