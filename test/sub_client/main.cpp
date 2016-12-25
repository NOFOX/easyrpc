#include <iostream>
#include <thread>
#include <easyrpc/easyrpc.hpp>
#include "user_define_classes.hpp"

void handle_weather(const std::string& str)
{
    std::cout << "weather: " << str << std::endl;
}

void handle_person_info(const std::vector<person_info_res>& res_vec)
{
    for (auto& res : res_vec)
    {
        std::cout << "card_id: " << res.card_id << std::endl;
        std::cout << "name: " << res.name << std::endl;
        std::cout << "age: " << res.age << std::endl;
        std::cout << "national: " << res.national << std::endl;
    }
}

class message_handle
{
public:
    void handle_news(const std::string& str)
    {
        std::cout << "news: " << str << std::endl;
    }
};

int main()
{
    try
    {
        message_handle m;
        easyrpc::sub_client sub_app;
        sub_app.connect({ "127.0.0.1", 50051 }).timeout(3000).run();
        sub_app.async_subscribe("weather", &handle_weather);
        sub_app.async_subscribe("person_info", &handle_person_info);
        sub_app.async_subscribe("news", &message_handle::handle_news, &m);
        sub_app.async_subscribe_raw("song", [](const std::string& str){ std::cout << str << std::endl; });
        std::cin.get();
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }

    return 0;
}


