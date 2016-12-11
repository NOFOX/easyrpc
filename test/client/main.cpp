#include <iostream>
#include <thread>
#include <easyrpc/easyrpc.hpp>
#include "user_define_classes.hpp"

EASYRPC_RPC_PROTOCOL_DEFINE(say_hello, void());
EASYRPC_RPC_PROTOCOL_DEFINE(echo, std::string(const std::string&));
EASYRPC_RPC_PROTOCOL_DEFINE(query_person_info, std::vector<person_info_res>(const person_info_req&));

int main()
{

    try
    {
#if 0
        easyrpc::pub_client pub_app;
        pub_app.connect({ "127.0.0.1", 50051 }).run();
        pub_app.publish("weather", "good");
#endif
        
#if 1
        easyrpc::sub_client sub_app;
        sub_app.connect({ "127.0.0.1", 50051 }).run();
        /* sub_app.subscribe("news", []{ std::cout << "Hello" << std::endl; }); */
        /* sub_app.subscribe("news", []{ std::cout << "Hello" << std::endl; }); */
        /* sub_app.subscribe("news", []{ std::cout << "Hello" << std::endl; }); */
        /* sub_app.subscribe("weather", []{ std::cout << "Hello" << std::endl; }); */
        /* sub_app.subscribe("weather", []{ std::cout << "Hello" << std::endl; }); */
        /* sub_app.subscribe("weather", []{ std::cout << "Hello" << std::endl; }); */
        sub_app.subscribe("weather", []{ std::cout << "Hello" << std::endl; });
        /* sub_app.cancel_subscribe("news"); */
        std::cin.get();
#endif
        
#if 0
        easyrpc::rpc_client rpc_app;
        rpc_app.connect({ "127.0.0.1", 50051 }).run();
        rpc_app.call(say_hello);
        std::string ret = rpc_app.call(echo, "Hello world");
        std::cout << ret << std::endl;

        person_info_req req { 12345678, "Jack" };
        auto vec = rpc_app.call(query_person_info, req);
        for (auto& res : vec)
        {
            std::cout << res.card_id << ", " << res.name << ", " << res.age << ", " << res.national << std::endl;
        }

        rpc_app.call_raw<easyrpc::one_way>("say_hi", "Hi");

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

#endif
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }

    return 0;
}


