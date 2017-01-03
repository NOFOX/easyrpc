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
    sub_client() : heartbeats_work_(heartbeats_ios_), heartbeats_timer_(heartbeats_ios_)
    {
        client_type_ = client_type::sub_client;
    }

    virtual ~sub_client()
    {
        stop();
    }

    virtual void run() override final
    {
        client_base::run();
        sync_connect();
        start_heartbeats_thread();
    }

    virtual void stop() override final
    {
        stop_heartbeats_thread();
        client_base::stop();
    }

    template<typename Function>
    void subscribe(const std::string& topic_name, const Function& func)
    {
        sync_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind(topic_name, func);
    }

    template<typename Function, typename Self>
    void subscribe(const std::string& topic_name, const Function& func, Self* self)
    {
        sync_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind(topic_name, func, self);
    }

    template<typename Function>
    void async_subscribe(const std::string& topic_name, const Function& func)
    {
        sync_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        async_call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind(topic_name, func);
    }

    template<typename Function, typename Self>
    void async_subscribe(const std::string& topic_name, const Function& func, Self* self)
    {
        sync_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        async_call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind(topic_name, func, self);
    }

    template<typename Function>
    void subscribe_raw(const std::string& topic_name, const Function& func)
    {
        sync_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind_raw(topic_name, func);
    }

    template<typename Function, typename Self>
    void subscribe_raw(const std::string& topic_name, const Function& func, Self* self)
    {
        sync_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind_raw(topic_name, func, self);
    }

    template<typename Function>
    void async_subscribe_raw(const std::string& topic_name, const Function& func)
    {
        sync_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        async_call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind_raw(topic_name, func);
    }

    template<typename Function, typename Self>
    void async_subscribe_raw(const std::string& topic_name, const Function& func, Self* self)
    {
        sync_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        async_call_one_way(topic_name, flag, subscribe_topic_flag);
        sub_router::singleton::get()->bind_raw(topic_name, func, self);
    }

    void cancel_subscribe(const std::string& topic_name)
    {
        sync_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        call_one_way(topic_name, flag, cancel_subscribe_topic_flag);
        sub_router::singleton::get()->unbind(topic_name);
    }

    void cancel_subscribe_raw(const std::string& topic_name)
    {
        sync_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        call_one_way(topic_name, flag, cancel_subscribe_topic_flag);
        sub_router::singleton::get()->unbind_raw(topic_name);
    }

    void async_cancel_subscribe(const std::string& topic_name)
    {
        sync_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        async_call_one_way(topic_name, flag, cancel_subscribe_topic_flag);
        sub_router::singleton::get()->unbind(topic_name);
    }

    void async_cancel_subscribe_raw(const std::string& topic_name)
    {
        sync_connect();
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

private:
    void async_read_head()
    {
        boost::asio::async_read(get_socket(), boost::asio::buffer(push_head_buf_), 
                                [this](boost::system::error_code ec, std::size_t)
        {
            if (!get_socket().is_open())
            {
                log_warn("Socket is not open");
                return;
            }

            if (ec)
            {
                log_warn(ec.message());
                return;
            }

            if (async_check_head())
            {
                async_read_protocol_and_body();
            }
        });
    }

    bool async_check_head()
    {
        memcpy(&push_head_, push_head_buf_, sizeof(push_head_buf_));
        unsigned int len = push_head_.protocol_len + push_head_.body_len;
        return (len > 0 && len < max_buffer_len) ? true : false;
    }

    void async_read_protocol_and_body()
    {
        protocol_and_body_.clear();
        protocol_and_body_.resize(push_head_.protocol_len + push_head_.body_len);
        boost::asio::async_read(get_socket(), boost::asio::buffer(protocol_and_body_), 
                                [this](boost::system::error_code ec, std::size_t)
        {
            async_read_head();

            if (!get_socket().is_open())
            {
                log_warn("Socket is not open");
                return;
            }

            if (ec)
            {
                log_warn(ec.message());
                return;
            }

            bool ok = sub_router::singleton::get()->route(std::string(&protocol_and_body_[0], push_head_.protocol_len), 
                                                         std::string(&protocol_and_body_[push_head_.protocol_len], push_head_.body_len), push_head_.mode);
            if (!ok)
            {
                log_warn("Router failed");
                return;
            }
        });
    }

    void heartbeats_timer()
    {
        std::cout << "###################################### heartbeats_timer" << std::endl;
        try
        {
            sync_connect();
            client_flag flag{ serialize_mode::serialize, client_type_ };
            async_call_one_way(heartbeats_flag, flag, heartbeats_flag);
        }
        catch (std::exception& e)
        {
            log_warn(e.what());
        }
    }

    void retry_subscribe()
    {
        try
        {
            for (auto& topic_name : sub_router::singleton::get()->get_all_topic())
            {
                client_flag flag{ serialize_mode::serialize, client_type_ };
                async_call_one_way(topic_name, flag, subscribe_topic_flag);
            }
        }
        catch (std::exception& e)
        {
            log_warn(e.what());
        }
    }

    void start_heartbeats_thread()
    {
        heatbeats_thread_ = std::make_unique<std::thread>([this]{ heartbeats_ios_.run(); });
        heartbeats_timer_.bind([this]{ heartbeats_timer(); });
        heartbeats_timer_.start(heartbeats_milli);
    }

    void stop_heartbeats_thread()
    {
        heartbeats_ios_.stop();
        if (heatbeats_thread_ != nullptr)
        {
            if (heatbeats_thread_->joinable())
            {
                heatbeats_thread_->join();
            }
        }
    }

    void sync_connect()
    {
        if (try_connect())
        {
            async_read_head();
            retry_subscribe();
        }
    }

private:
    char push_head_buf_[push_header_len];
    push_header push_head_;
    std::vector<char> protocol_and_body_;

    boost::asio::io_service heartbeats_ios_;
    boost::asio::io_service::work heartbeats_work_;
    std::unique_ptr<std::thread> heatbeats_thread_;
    atimer<> heartbeats_timer_;
};

}

#endif
