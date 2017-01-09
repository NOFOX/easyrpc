#include <iostream>
#include <thread>
#include <easyrpc/easyrpc.hpp>
#include "user_define_classes.hpp"

EASYRPC_RPC_PROTOCOL_DEFINE(say_hello, void());
EASYRPC_RPC_PROTOCOL_DEFINE(echo, std::string(const std::string&));
EASYRPC_RPC_PROTOCOL_DEFINE(query_person_info, std::vector<person_info_res>(const person_info_req&));

easyrpc::async_rpc_client app;

void test_func()
{
    while (true)
    {
        app.async_call(echo, "Hello").result([](const auto& ret)
        {
            std::cout << ret << std::endl;
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

int main()
{
#if 0
    try
    {
        app.connect({ "127.0.0.1", 50051 }).timeout(3000).run();
        app.async_call(echo, "Hello").result([](const auto& ret)
        {
            std::cout << ret << std::endl;
        });

        app.async_call(echo, "Hello").result([](const auto& ret)
        {
            std::cout << ret << std::endl;
        });
        std::cout << "#############end#######################" << std::endl;
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }
    std::cin.get();
#endif

    try
    {
        app.connect({ "127.0.0.1", 50051 }).timeout(3000).run();

        std::thread t(test_func);
        std::thread t2(test_func);

        t.join();
        t2.join();
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }
    std::cin.get();
    return 0;
}


