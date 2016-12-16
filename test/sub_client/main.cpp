#include <iostream>
#include <thread>
#include <easyrpc/easyrpc.hpp>

void handle_weather(const std::string& str)
{
    std::cout << "weather: " << str << std::endl;
}

int main()
{
    try
    {
        easyrpc::sub_client sub_app;
        sub_app.connect({ "127.0.0.1", 50051 }).run();
        sub_app.subscribe("weather", &handle_weather);
        std::cin.get();
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }

    return 0;
}


