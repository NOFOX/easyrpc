#include <iostream>
#include <thread>
#include <easyrpc/easyrpc.hpp>
#include "user_define_classes.hpp"

EASYRPC_RPC_PROTOCOL_DEFINE(say_hello, void());
EASYRPC_RPC_PROTOCOL_DEFINE(echo, std::string(const std::string&));
EASYRPC_RPC_PROTOCOL_DEFINE(query_person_info, std::vector<person_info_res>(const person_info_req&));

int main()
{
    easyrpc::async_rpc_client app;
    try
    {
        app.connect({ "127.0.0.1", 50051 }).timeout(3000).run();
        app.async_call(echo, "Hello").result([](const auto& ret)
        {
            std::cout << ret << std::endl;
        });
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }
    std::cin.get();

    return 0;
}


