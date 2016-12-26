#ifndef _RPC_ASYNC_CALL_H
#define _RPC_ASYNC_CALL_H

#include <iostream>
#include <string>

namespace easyrpc
{

template<typename ReturnType>
class rpc_async_call
{
public:
    rpc_async_call(const std::string& buffer) : buffer_(buffer) {}

    template<typename Function>
    void result(const Function& func)
    {
        std::cout << buffer_ << std::endl; 
        func();
    }

private:
    std::string buffer_;
};

}

#endif
