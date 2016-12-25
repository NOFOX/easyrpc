#ifndef _SUB_CLIENT_H
#define _SUB_CLIENT_H

#include "client_base.hpp"
#include "sub_router.hpp"

namespace easyrpc
{

class sub_client : public client_base
{
public:
    sub_client(const sub_client&) = delete;
    sub_client& operator=(const sub_client&) = delete;
    sub_client()
    {
        client_type_ = client_type::sub_client;
    }

    template<typename Function>
    void subscribe(const std::string& topic_name, const Function& func)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind(topic_name, func);
    }

    template<typename Function, typename Self>
    void subscribe(const std::string& topic_name, const Function& func, Self* self)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind(topic_name, func, self);
    }

    template<typename Function>
    void async_subscribe(const std::string& topic_name, const Function& func)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        async_call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind(topic_name, func);
    }

    template<typename Function, typename Self>
    void async_subscribe(const std::string& topic_name, const Function& func, Self* self)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        async_call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind(topic_name, func, self);
    }

    template<typename Function>
    void subscribe_raw(const std::string& topic_name, const Function& func)
    {
        try_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind_raw(topic_name, func);
    }

    template<typename Function, typename Self>
    void subscribe_raw(const std::string& topic_name, const Function& func, Self* self)
    {
        try_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind_raw(topic_name, func, self);
    }

    template<typename Function>
    void async_subscribe_raw(const std::string& topic_name, const Function& func)
    {
        try_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        async_call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind_raw(topic_name, func);
    }

    template<typename Function, typename Self>
    void async_subscribe_raw(const std::string& topic_name, const Function& func, Self* self)
    {
        try_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        async_call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind_raw(topic_name, func, self);
    }

    void cancel_subscribe(const std::string& topic_name)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        call_one_way(topic_name, flag, cancel_subscribe_topic_flag);
        sub_router::singleton::get()->unbind(topic_name);
    }

    void cancel_subscribe_raw(const std::string& topic_name)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        call_one_way(topic_name, flag, cancel_subscribe_topic_flag);
        sub_router::singleton::get()->unbind_raw(topic_name);
    }

    void async_cancel_subscribe(const std::string& topic_name)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        async_call_one_way(topic_name, flag, cancel_subscribe_topic_flag);
        sub_router::singleton::get()->unbind(topic_name);
    }

    void async_cancel_subscribe_raw(const std::string& topic_name)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        async_call_one_way(topic_name, flag, cancel_subscribe_topic_flag);
        sub_router::singleton::get()->unbind_raw(topic_name);
    }

    bool is_subscribe(const std::string& topic_name)
    {
        return sub_router::singleton::get()->is_bind(topic_name);
    }

    bool is_subscribe_raw(const std::string& topic_name)
    {
        return sub_router::singleton::get()->is_bind_raw(topic_name);
    }
};

}

#endif
