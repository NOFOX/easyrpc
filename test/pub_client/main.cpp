#include <iostream>
#include <thread>
#include <easyrpc/easyrpc.hpp>

int main()
{
    try
    {
        easyrpc::pub_client client;
        client.connect({ "127.0.0.1", 50051 }).run();
        while (true)
        {
            client.publish("weather", "good");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }

    return 0;
}


