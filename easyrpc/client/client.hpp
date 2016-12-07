#ifndef _CLIENT_H
#define _CLIENT_H

#include "base/string_util.hpp"
#include "protocol.hpp"
#include "rpc_session.hpp"

namespace easyrpc
{

class client
{
public:
    client() = default;
    client(const client&) = delete;
    client& operator=(const client&) = delete;
    ~client()
    {
        stop();
    }

    client& connect(const std::string& address)
    {
        std::vector<std::string> token = string_util::split(address, ":");
        if (token.size() != 2)
        {
            throw std::invalid_argument("Address format error");
        }
        return connect(token[0], token[1]);
    }

    client& connect(const std::string& ip, unsigned short port)
    {
        return connect(ip, std::to_string(port));
    }

    client& connect(const std::string& ip, const std::string& port)
    {
        session_.connect(ip, port);
        return *this;
    }

    client& timeout(std::size_t timeout_milli)
    {
        session_.timeout(timeout_milli);
        return *this;
    }

    void run()
    {
        session_.run();
    }

    void stop()
    {
        session_.stop();
    }

    template<typename Protocol, typename... Args>
    typename std::enable_if<std::is_void<typename Protocol::return_type>::value, typename Protocol::return_type>::type
    call(const Protocol& protocol, Args&&... args)
    {
        client_flag flag{ serialize_mode::serialize, call_mode::rpc_mode };
        session_.call_one_way(protocol.name(), flag, serialize(std::forward<Args>(args)...));
    }

    template<typename Protocol, typename... Args>
    typename std::enable_if<!std::is_void<typename Protocol::return_type>::value, typename Protocol::return_type>::type
    call(const Protocol& protocol, Args&&... args)
    {
        client_flag flag{ serialize_mode::serialize, call_mode::rpc_mode };
        auto ret = session_.call_two_way(protocol.name(), flag, serialize(std::forward<Args>(args)...));
        return protocol.deserialize(std::string(&ret[0], ret.size()));
    }

    template<typename ReturnType>
    typename std::enable_if<std::is_same<ReturnType, one_way>::value>::type 
    call_raw(const std::string& protocol, const std::string& body)
    {
        client_flag flag{ serialize_mode::non_serialize, call_mode::rpc_mode };
        session_.call_one_way(protocol, flag, body);
    }

    template<typename ReturnType>
    typename std::enable_if<std::is_same<ReturnType, two_way>::value, std::string>::type 
    call_raw(const std::string& protocol, const std::string& body)
    {
        client_flag flag{ serialize_mode::non_serialize, call_mode::rpc_mode };
        auto ret = session_.call_two_way(protocol, flag, body);
        return std::string(&ret[0], ret.size());
    }

    template<typename... Args>
    void publish(const std::string& topic_name, Args&&... args)
    {
        client_flag flag{ serialize_mode::serialize, call_mode::pub_mode };
        session_.call_one_way(topic_name, flag, serialize(std::forward<Args>(args)...));
    }

    void publish_raw(const std::string& topic_name, const std::string& body)
    {
        client_flag flag{ serialize_mode::non_serialize, call_mode::pub_mode };
        session_.call_one_way(topic_name, flag, body);
    }

    template<typename Function>
    void subscribe(const std::string& topic_name, const Function& func)
    {
        try
        {
            client_flag flag{ serialize_mode::serialize, call_mode::sub_mode };
            session_.call_one_way(topic_name, flag, subscribe_topic_flag);
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
            client_flag flag{ serialize_mode::serialize, call_mode::sub_mode };
            session_.call_one_way(topic_name, flag, subscribe_topic_flag);
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
            client_flag flag{ serialize_mode::serialize, call_mode::sub_mode };
            session_.call_one_way(topic_name, flag, cancel_subscribe_topic_flag);
        }
        catch (std::exception& e)
        {
            throw std::runtime_error(e.what());
        }
    }

private:
    rpc_session session_;
};

}

#endif
