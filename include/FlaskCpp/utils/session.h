#ifndef FLASKCPP_UTILS_SESSION_H
#define FLASKCPP_UTILS_SESSION_H

#include "urlSafeSerializer.h"

namespace flaskcpp
{
    struct Session
    {

        std::map<std::string, std::string> data;

        std::string get(const std::string& key, const std::string& default_value)
        {
            auto it = data.find(key);
            if (it != data.end())
            {
                return it->second;
            }
            return default_value;
        }

    };

}


#endif