#include <iostream>
#include <string>
#include <thread>
#include <easyrpc/easyrpc.hpp>
#include "user_define_classes.hpp"

std::vector<person_info_res> query_person_info(const person_info_req& req)
{
    std::vector<person_info_res> vec;
    for (int i = 0; i < 2; ++i)
    {
        person_info_res res;
        res.card_id = req.card_id;
        res.name = req.name;
        res.age = 20;
        res.national = "han";
        vec.emplace_back(std::move(res));
    }
    return vec;
}

#ifdef ENABLE_JSON
std::string call_person(const std::string& str)
{
    std::cout << str << std::endl;
    DeSerializer dr;
    person_info_req req;
    dr.Parse(str);
    dr.Deserialize(req);

    person_info_res res;
    res.card_id = req.card_id;
    res.name = req.name;
    res.age = 20;
    res.national = "han";
    Serializer sr;
    sr.Serialize(res);
    return sr.GetString();
}
#endif

class message_handle
{
public:
    std::string echo(const std::string& str)
    {
        return str;
    }
};

void sayHi(const std::string& str)
{
    std::cout << str << std::endl;
}

int main()
{
    message_handle hander;
    easyrpc::server app;
    try
    {
        app.bind("say_hello", []{ std::cout << "Hello" << std::endl; });
        app.bind("echo", &message_handle::echo, &hander);
        app.bind("query_person_info", &query_person_info);
        app.bind_raw("say_hi", &sayHi);

#ifdef ENABLE_JSON
        app.bind_raw("call_person", &call_person);
#endif
        std::vector<easyrpc::endpoint> ep;
        ep.emplace_back(easyrpc::endpoint{ "127.0.0.1", 50051 });
        ep.emplace_back(easyrpc::endpoint{ "127.0.0.1", 50052 });
        app.listen(ep).multithreaded(10).run();
    }
    catch (std::exception& e)
    {
        easyrpc::log_warn(e.what());
        return 0;
    }

    std::cin.get();
    return 0;
}
