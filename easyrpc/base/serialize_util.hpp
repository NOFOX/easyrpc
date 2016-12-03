#ifndef _SERIALIZE_UTIL_H
#define _SERIALIZE_UTIL_H

#include <easypack/easypack.hpp>

namespace easyrpc
{

template<typename... Args>
std::string serialize(Args... args) 
{
    easypack::pack p;
    p.pack_args(std::forward<Args>(args)...);
    return p.get_string();
}

}

#endif
