#include <iostream>
#include <thread>
#include <easyrpc/easyrpc.hpp>

int main()
{
    try
    {
        easyrpc::sub_client sub_app;
        sub_app.connect({ "127.0.0.1", 50051 }).run();
        sub_app.subscribe("weather", []{ std::cout << "Hello" << std::endl; });
        std::cin.get();
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }

    return 0;
}


