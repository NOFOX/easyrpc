#ifndef _RPC_CLIENT_H
#define _RPC_CLIENT_H

#include "protocol.hpp"
#include "client_base.hpp"

namespace easyrpc
{

class rpc_client : public client_base
{
public:
    rpc_client(const rpc_client&) = delete;
    rpc_client& operator=(const rpc_client&) = delete;
    rpc_client() 
    {
        client_type_ = client_type::rpc_client;
    }

    virtual void run() override final
    {
        client_base::run();
        try_connect();
    }

    template<typename Protocol, typename... Args>
    typename std::enable_if<std::is_void<typename Protocol::return_type>::value, typename Protocol::return_type>::type
    call(const Protocol& protocol, Args&&... args)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        call_one_way(protocol.name(), flag, serialize(std::forward<Args>(args)...));
    }

    template<typename Protocol, typename... Args>
    typename std::enable_if<!std::is_void<typename Protocol::return_type>::value, typename Protocol::return_type>::type
    call(const Protocol& protocol, Args&&... args)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        auto ret = call_two_way(protocol.name(), flag, serialize(std::forward<Args>(args)...));
        return protocol.deserialize(std::string(&ret[0], ret.size()));
    }

    template<typename ReturnType>
    typename std::enable_if<std::is_same<ReturnType, one_way>::value>::type 
    call_raw(const std::string& protocol, const std::string& body)
    {
        try_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        call_one_way(protocol, flag, body);
    }

    template<typename ReturnType>
    typename std::enable_if<std::is_same<ReturnType, two_way>::value, std::string>::type 
    call_raw(const std::string& protocol, const std::string& body)
    {
        try_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        auto ret = call_two_way(protocol, flag, body);
        return std::string(&ret[0], ret.size());
    }

    template<typename ReturnType>
    class rpc_task
    {
    public:
        using task_t = std::function<void()>; 
        rpc_task(const std::string& buffer) : buffer_(buffer) {}

        template<typename Function>
        void result(const Function& func)
        {
            std::cout << buffer_ << std::endl;
            task_ = [&func]
            {
                ReturnType ret;
                ret = "nihao";
                return func(ret); 
            };
            func("Hello world");
            task_();
        }

    private:
        std::string buffer_;
        task_t task_;
    };

    template<typename Protocol, typename... Args>
    auto async_call(const Protocol& protocol, Args&&... args)
    {
        std::string body = serialize(std::forward<Args>(args)...);
        std::string proto_name = protocol.name();
        unsigned int protocol_len = static_cast<unsigned int>(proto_name.size());
        unsigned int body_len = static_cast<unsigned int>(body.size());
        if (protocol_len + body_len > max_buffer_len)
        {
            throw std::runtime_error("Send data is too big");
        }

        /* try_connect(); */
        client_flag flag{ serialize_mode::serialize, client_type_ };
        std::string buffer = get_buffer(request_header{ protocol_len, body_len, flag }, proto_name, body);
        using return_type = typename Protocol::return_type;
        return rpc_task<return_type>{ buffer };
    }
};

}

#endif
