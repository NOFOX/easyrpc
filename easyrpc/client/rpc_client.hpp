#ifndef _RPC_CLIENT_H
#define _RPC_CLIENT_H

#include <unordered_map>
#include <mutex>
#include "base/common_util.hpp"
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
        request_content content;
        content.protocol = protocol.name();
        content.body = serialize(std::forward<Args>(args)...);
        call_one_way(flag, content);
    }

    template<typename Protocol, typename... Args>
    typename std::enable_if<!std::is_void<typename Protocol::return_type>::value, typename Protocol::return_type>::type
    call(const Protocol& protocol, Args&&... args)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        request_content content;
        content.protocol = protocol.name();
        content.body = serialize(std::forward<Args>(args)...);
        auto ret = call_two_way(flag, content);
        return protocol.deserialize(std::string(&ret[0], ret.size()));
    }

    template<typename ReturnType>
    typename std::enable_if<std::is_same<ReturnType, one_way>::value>::type 
    call_raw(const std::string& protocol, const std::string& body)
    {
        try_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        request_content content;
        content.protocol = protocol;
        content.body = body;
        call_one_way(flag, content);
    }

    template<typename ReturnType>
    typename std::enable_if<std::is_same<ReturnType, two_way>::value, std::string>::type 
    call_raw(const std::string& protocol, const std::string& body)
    {
        try_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        request_content content;
        content.protocol = protocol;
        content.body = body;
        auto ret = call_two_way(flag, content);
        return std::string(&ret[0], ret.size());
    }

    using task_t = std::function<void(const std::string&)>; 
    template<typename ReturnType>
    class rpc_task
    {
    public:
        rpc_task(const client_flag& flag, const request_content& content, rpc_client* client) 
            : flag_(flag), content_(content), client_(client) {}

        template<typename Function>
        void result(const Function& func)
        {
            task_ = [&func](const std::string& body)
            {
                std::cout << "body: " << body << std::endl;
                return func(deserialize(std::string(&body[0], body.size()))); 
            };
            /* task_("Hello C++"); */
            client_->async_call_one_way(flag_, content_);
            client_->add_bind_func(content_.call_id, task_);
        }

    private:
        ReturnType deserialize(const std::string& text) 
        {
            easypack::unpack up(text);
            ReturnType ret;
            up.unpack_args(ret);
            return std::move(ret);
        }

    private:
        client_flag flag_;
        request_content content_;
        task_t task_;
        rpc_client* client_;
    };

    template<typename Protocol, typename... Args>
    auto async_call(const Protocol& protocol, Args&&... args)
    {
        request_content content;
        content.call_id = gen_uuid();
        content.protocol = protocol.name();
        content.body = serialize(std::forward<Args>(args)...);

        client_flag flag{ serialize_mode::serialize, client_type_ };
        /* try_connect(); */
        using return_type = typename Protocol::return_type;
        return rpc_task<return_type>{ flag, content, this };
    }

    void add_bind_func(const std::string& call_id, const task_t& task)
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        task_map_.emplace(call_id, task);
    }

private:
    std::unordered_map<std::string, task_t> task_map_;
    std::mutex task_mutex_;
};

}

#endif
