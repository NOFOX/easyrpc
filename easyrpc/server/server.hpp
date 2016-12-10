#ifndef _SERVER_H
#define _SERVER_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "router.hpp"
#include "tcp_endpoint.hpp"

namespace easyrpc
{

class server
{
public:
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    server()
    {
        set_pub_sub_callback();
    }

    ~server()
    {
        stop();
    }

    server& listen(const endpoint& ep)
    {
        endpoint_vec_.emplace_back(ep);
        return *this;
    }

    server& listen(const std::vector<endpoint>& ep_vec)
    {
        endpoint_vec_ = ep_vec;
        return *this;
    }

    server& timeout(std::size_t timeout_milli)
    {
        timeout_milli_ = timeout_milli;
        return *this;
    }

    server& multithreaded(std::size_t num)
    {
        thread_num_ = num;
        return *this;
    }

    void run()
    {
        router::singleton::get()->multithreaded(thread_num_);
        listen();
        accept();
        io_service_pool::singleton::get()->run();
    }

    void stop()
    {
        io_service_pool::singleton::get()->stop();
    }

    template<typename Function>
    void bind(const std::string& protocol, const Function& func)
    {
        router::singleton::get()->bind(protocol, func);
    }

    template<typename Function, typename Self>
    void bind(const std::string& protocol, const Function& func, Self* self)
    {
        router::singleton::get()->bind(protocol, func, self); 
    }

    void unbind(const std::string& protocol)
    {
        router::singleton::get()->unbind(protocol);
    }

    bool is_bind(const std::string& protocol)
    {
        return router::singleton::get()->is_bind(protocol);
    }

    template<typename Function>
    void bind_raw(const std::string& protocol, const Function& func)
    {
        router::singleton::get()->bind_raw(protocol, func);
    }

    template<typename Function, typename Self>
    void bind_raw(const std::string& protocol, const Function& func, Self* self)
    {
        router::singleton::get()->bind_raw(protocol, func, self); 
    }

    void unbind_raw(const std::string& protocol)
    {
        router::singleton::get()->unbind_raw(protocol);
    }

    bool is_bind_raw(const std::string& protocol)
    {
        return router::singleton::get()->is_bind_raw(protocol);
    }

private:
    void listen()
    {
        auto route_func = std::bind(&server::route, this, 
                                    std::placeholders::_1, std::placeholders::_2,
                                    std::placeholders::_3, std::placeholders::_4);
        auto remove_all_topic_func = std::bind(&server::remove_all_topic, this, std::placeholders::_1);
        for (auto& ep : endpoint_vec_)
        {
            auto endpoint = std::make_shared<tcp_endpoint>(route_func, remove_all_topic_func);
            endpoint->listen(ep.ip, ep.port);
            tcp_endpoint_vec_.emplace_back(endpoint);
        }
    }

    void accept()
    {
        for (auto& endpoint : tcp_endpoint_vec_)
        {
            endpoint->accept();
        }
    }

    void set_pub_sub_callback()
    {
        router::singleton::get()->publisher_coming_ = std::bind(&server::publisher_coming, 
                                                                this, std::placeholders::_1, std::placeholders::_2);
        router::singleton::get()->subscriber_coming_ = std::bind(&server::subscriber_coming, 
                                                                 this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    bool route(const std::string& protocol, const std::string& body,
               const client_flag& flag, const connection_ptr& conn)
    {
        return router::singleton::get()->route(protocol, body, flag, conn); 
    }

    void publisher_coming(const std::string& topic_name, const std::string& body)
    {
        std::cout << "pub topic_name: " << topic_name << ", body: " << body << std::endl;
    }

    void subscriber_coming(const std::string& topic_name, const std::string& body, const connection_ptr& conn)
    {
        if (body == subscribe_topic_flag)
        {
            add_topic(topic_name, conn);
        }
        else if (body == cancel_subscribe_topic_flag)
        {
            remove_topic(topic_name, conn);
        }
    }

    void add_topic(const std::string& topic_name, const connection_ptr& conn)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto range = topic_map_.equal_range(topic_name);
        for (auto iter = range.first; iter != range.second; ++iter)
        {
            if (iter->second.lock() == conn)
            {
                /* std::cout << "has topic: " << topic_name << std::endl; */
                return;
            }
        }
        /* std::cout << "insert topic: " << topic_name << std::endl; */
        topic_map_.emplace(topic_name, conn);
    }

    void remove_topic(const std::string& topic_name, const connection_ptr& conn)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = topic_map_.find(topic_name);
        if (iter != topic_map_.end())
        {
            auto range = topic_map_.equal_range(iter->first);
            while (range.first != range.second)
            {
                if (range.first->second.lock() == conn)
                {
                    range.first = topic_map_.erase(range.first);
                }
                else
                {
                    ++range.first;
                }
            }
        }
    }

    void remove_all_topic(const connection_ptr& conn)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto begin = topic_map_.begin();
        while (begin != topic_map_.end())
        {
            if (begin->second.lock() == conn)
            {
                std::cout << "remove topic: " << begin->first << std::endl;
                begin = topic_map_.erase(begin);
            }
            else
            {
                ++begin;
            }
        }
    }

private:
    std::size_t timeout_milli_ = 0;
    std::size_t thread_num_ = 1;

    std::vector<endpoint> endpoint_vec_;
    std::vector<std::shared_ptr<tcp_endpoint>> tcp_endpoint_vec_;
    std::unordered_multimap<std::string, connection_weak_ptr> topic_map_;
    std::mutex mutex_;
};

}

#endif
