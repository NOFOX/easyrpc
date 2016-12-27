#include <iostream>
#include <thread>
#include <easyrpc/easyrpc.hpp>
#include "user_define_classes.hpp"

EASYRPC_RPC_PROTOCOL_DEFINE(say_hello, void());
EASYRPC_RPC_PROTOCOL_DEFINE(echo, std::string(const std::string&));
EASYRPC_RPC_PROTOCOL_DEFINE(query_person_info, std::vector<person_info_res>(const person_info_req&));

easyrpc::rpc_client rpc_app;

void test_func()
{
    while (true)
    {
        try
        {
            std::string ret = rpc_app.call(echo, "Hello world");
            std::cout << ret << std::endl;

            person_info_req req { 12345678, "Jack" };
            auto vec = rpc_app.call(query_person_info, req);
            for (auto& res : vec)
            {
                std::cout << res.card_id << ", " << res.name << ", " << res.age << ", " << res.national << std::endl;
            }
        }
        catch (std::exception& e)
        {
            easyrpc::log_warn(e.what());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void test_func2()
{
    while (true)
    {
        try
        {
            std::string ret = rpc_app.call(echo, "Hello world");
            std::cout << ret << std::endl;

            person_info_req req { 12345678, "Jack" };
            auto vec = rpc_app.call(query_person_info, req);
            for (auto& res : vec)
            {
                std::cout << res.card_id << ", " << res.name << ", " << res.age << ", " << res.national << std::endl;
            }
        }
        catch (std::exception& e)
        {
            easyrpc::log_warn(e.what());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

int main()
{
    try
    {
        /* rpc_app.connect({ "127.0.0.1", 50051 }).timeout(3000).run(); */
        rpc_app.async_call(echo, "Hello").result([](const auto& ret)
        {
            std::cout << ret << std::endl;
        });
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }

#if 0
    try
    {
        rpc_app.connect({ "127.0.0.1", 50051 }).timeout(3000).run();
        rpc_app.call(say_hello);
        std::string ret = rpc_app.call(echo, "Hello world");
        std::cout << ret << std::endl;

        person_info_req req { 12345678, "Jack" };
        auto vec = rpc_app.call(query_person_info, req);
        for (auto& res : vec)
        {
            std::cout << res.card_id << ", " << res.name << ", " << res.age << ", " << res.national << std::endl;
        }
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }

    std::thread t(test_func);
    std::thread t2(test_func2);

    t.join();
    t2.join();
#endif

#if 0
    try
    {
        easyrpc::rpc_client rpc_app;
        rpc_app.connect({ "127.0.0.1", 50051 }).timeout(3000).run();
        rpc_app.call(say_hello);
        std::string ret = rpc_app.call(echo, "Hello world");
        std::cout << ret << std::endl;

        person_info_req req { 12345678, "Jack" };
        auto vec = rpc_app.call(query_person_info, req);
        for (auto& res : vec)
        {
            std::cout << res.card_id << ", " << res.name << ", " << res.age << ", " << res.national << std::endl;
        }

        /* rpc_app.call_raw<easyrpc::one_way>("say_hi", "Hi"); */

#ifdef ENABLE_JSON
        person_info_req req2 { 12345678, "Jack" };
        Serializer sr;
        sr.Serialize(req2);
        std::string str = rpc_app.call_raw<easyrpc::two_way>("call_person", sr.GetString());
        std::cout << str << std::endl;

        person_info_res res2;
        DeSerializer dr;
        dr.Parse(str);
        dr.Deserialize(res2);
        std::cout << res2.card_id << ", " << res2.name << ", " << res2.age << ", " << res2.national << std::endl;
#endif
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }
#endif

    return 0;
}


