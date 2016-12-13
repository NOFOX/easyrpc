#ifndef _SUB_CLIENT_H
#define _SUB_CLIENT_H

#include "client_base.hpp"

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
        try
        {
            client_flag flag{ serialize_mode::serialize, client_type_ };
            call_one_way(topic_name, flag, subscribe_topic_flag);
            do_read();
        }
        catch (std::exception& e)
        {
            throw std::runtime_error(e.what());
        }
    }

    template<typename Function, typename Self>
    void subscribe(const std::string& topic_name, const Function& func, Self* self)
    {
        try
        {
            client_flag flag{ serialize_mode::serialize, client_type_ };
            call_one_way(topic_name, flag, subscribe_topic_flag);
            do_read();
        }
        catch (std::exception& e)
        {
            throw std::runtime_error(e.what());
        }
    }

    template<typename Function>
    void subscribe_raw(const std::string& topic_name, const Function& func)
    {
    }

    template<typename Function, typename Self>
    void subscribe_raw(const std::string& topic_name, const Function& func, Self* self)
    {
    }

    void cancel_subscribe(const std::string& topic_name)
    {
        try
        {
            client_flag flag{ serialize_mode::serialize, client_type_ };
            call_one_way(topic_name, flag, cancel_subscribe_topic_flag);
        }
        catch (std::exception& e)
        {
            throw std::runtime_error(e.what());
        }
    }
};

}

#endif
