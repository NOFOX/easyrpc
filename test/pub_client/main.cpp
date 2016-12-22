#include <iostream>
#include <thread>
#include <easyrpc/easyrpc.hpp>
#include "user_define_classes.hpp"

std::vector<person_info_res> get_person_info()
{
    std::vector<person_info_res> res_vec;
    person_info_res res;
    res.card_id = 12345;
    res.name = "Jack";
    res.age = 20;
    res.national = "han";

    person_info_res res2;
    res2.card_id = 56789;
    res2.name = "Tom";
    res2.age = 21;
    res2.national = "han";

    res_vec.emplace_back(res);
    res_vec.emplace_back(res2);
    return std::move(res_vec);
}

int main()
{
    easyrpc::pub_client client;
    try
    {
        /* client.connect({ "127.0.0.1", 50051 }).run(); */
        client.connect({ "127.0.0.1", 50051 }).timeout(3000).run();
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }

    while (true)
    {
        try
        {
            client.publish("weather", "The weather is good");
            client.publish_raw("song", "My heart will go on");
            /* client.publish("news", "good news"); */
            /* client.publish("person_info", get_person_info()); */
        }
        catch (std::exception& e)
        {
            easyrpc::log_warn(e.what());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
    return 0;
}


