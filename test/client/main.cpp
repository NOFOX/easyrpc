#include <iostream>
#include <thread>
#include <easyrpc/easyrpc.hpp>
#include "user_define_classes.hpp"

EASYRPC_RPC_PROTOCOL_DEFINE(say_hello, void());
EASYRPC_RPC_PROTOCOL_DEFINE(echo, std::string(const std::string&));
EASYRPC_RPC_PROTOCOL_DEFINE(query_person_info, std::vector<person_info_res>(const person_info_req&));

int main()
{
    easyrpc::client app;

    try
    {
        /* app.connect({ "127.0.0.1", 50051 }).run(); */
        app.connect({ "127.0.0.1", 50052 }).run();
        
        app.publish("weather", "good");
        app.subscribe("news", []{ std::cout << "Hello" << std::endl; });
        app.subscribe("news", []{ std::cout << "Hello" << std::endl; });
        app.subscribe("news", []{ std::cout << "Hello" << std::endl; });
        app.subscribe("weather", []{ std::cout << "Hello" << std::endl; });
        app.subscribe("weather", []{ std::cout << "Hello" << std::endl; });
        app.subscribe("weather", []{ std::cout << "Hello" << std::endl; });
        app.subscribe("weather", []{ std::cout << "Hello" << std::endl; });
        /* app.cancel_subscribe("news"); */
#if 0
        app.call(say_hello);
        std::string ret = app.call(echo, "Hello world");
        std::cout << ret << std::endl;

        person_info_req req { 12345678, "Jack" };
        auto vec = app.call(query_person_info, req);
        for (auto& res : vec)
        {
            EXPECT_EQ(req.card_id, res.card_id);
            EXPECT_STREQ(req.name.c_str(), res.name.c_str());
            EXPECT_EQ(20, res.age);
            EXPECT_STREQ("han", res.national.c_str());
        }

        app.call_raw<easyrpc::one_way>("say_hi", "Hi");

#ifdef ENABLE_JSON
        person_info_req req2 { 12345678, "Jack" };
        Serializer sr;
        sr.Serialize(req2);
        std::string str = app.call_raw<easyrpc::two_way>("call_person", sr.GetString());
        std::cout << str << std::endl;

        person_info_res res2;
        DeSerializer dr;
        dr.Parse(str);
        dr.Deserialize(res2);
        EXPECT_EQ(req2.card_id, res2.card_id);
        EXPECT_STREQ(req2.name.c_str(), res2.name.c_str());
        EXPECT_EQ(20, res2.age);
        EXPECT_STREQ("han", res2.national.c_str()); 
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


