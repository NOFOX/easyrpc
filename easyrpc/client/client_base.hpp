#ifndef _CLIENT_BASE_H
#define _CLIENT_BASE_H

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <boost/asio.hpp>
#include "base/header.hpp"
#include "base/atimer.hpp"
#include "base/scope_guard.hpp"
#include "base/logger.hpp"
#include "base/async_send_queue.hpp"
#include "sub_router.hpp"

namespace easyrpc
{

class client_base
{
public:
    client_base() : work_(ios_), socket_(ios_), 
    timer_work_(timer_ios_), timer_(timer_ios_), is_connected_(false),
    heartbeats_work_(heartbeats_ios_), heartbeats_timer_(heartbeats_ios_){}
    virtual ~client_base()
    {
        stop();
    }

    client_base& connect(const endpoint& ep)
    {
        boost::asio::ip::tcp::resolver resolver(ios_);
        boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), ep.ip, std::to_string(ep.port));
        endpoint_iter_ = resolver.resolve(query);
        return *this;
    }

    client_base& timeout(std::size_t timeout_milli)
    {
        timeout_milli_ = timeout_milli;
        return *this;
    }

    void run()
    {
        start_ios_thread();
        try_connect();
        start_timer_thread();
        start_heartbeats_thread();
    }

    void stop()
    {
        stop_heartbeats_thread();
        stop_timer_thread();
        stop_ios_thread();
    }

    void call_one_way(const std::string& protocol, const client_flag& flag, const std::string& body)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        write(protocol, flag, body);
    }

    std::vector<char> call_two_way(const std::string& protocol, const client_flag& flag, const std::string& body)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        write(protocol, flag, body);
        return read();
    }

    void async_call_one_way(const std::string& protocol, const client_flag& flag, const std::string& body)
    {
        async_write(protocol, flag, body);
    }

    void disconnect()
    {
        is_connected_ = false;
        if (socket_.is_open())
        {
            boost::system::error_code ignore_ec;
            socket_.shutdown(boost::asio::socket_base::shutdown_both, ignore_ec);
            socket_.close(ignore_ec);
        }
    }

    void try_connect()
    {
        if (!is_connected_)
        {
            std::lock_guard<std::mutex> lock(conn_mutex_);
            if (!is_connected_)
            {
                connect();
                is_connected_ = true;
                if (client_type_ == client_type::sub_client)
                {
                    do_read();
                    retry_subscribe();
                }
            }
        }
    }

protected:
    std::string get_buffer(const request_header& head, const std::string& protocol, const std::string& body)
    {
        std::string buffer;
        buffer.append(reinterpret_cast<const char*>(&head), sizeof(head));
        buffer.append(protocol);
        buffer.append(body);
        return std::move(buffer);
    }

private:
    void connect()
    {
        auto begin_time = std::chrono::high_resolution_clock::now();
        while (true)
        {
            try
            {
                boost::asio::connect(socket_, endpoint_iter_);
                break;
            }
            catch (std::exception& e)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                auto end_time = std::chrono::high_resolution_clock::now();
                auto elapsed_time = end_time - begin_time;
                if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count() > static_cast<long>(timeout_milli_))
                {
                    throw std::runtime_error(e.what());
                }
            }
        }
    }

    void do_read()
    {
        async_read_head();
    }

    void write(const std::string& protocol, const client_flag& flag, const std::string& body)
    {
        unsigned int protocol_len = static_cast<unsigned int>(protocol.size());
        unsigned int body_len = static_cast<unsigned int>(body.size());
        if (protocol_len + body_len > max_buffer_len)
        {
            throw std::runtime_error("Send data is too big");
        }

        std::string buffer = get_buffer(request_header{ protocol_len, body_len, flag }, protocol, body);
        write_impl(buffer);
    }

    void async_write(const std::string& protocol, const client_flag& flag, const std::string& body)
    {
        unsigned int protocol_len = static_cast<unsigned int>(protocol.size());
        unsigned int body_len = static_cast<unsigned int>(body.size());
        if (protocol_len + body_len > max_buffer_len)
        {
            throw std::runtime_error("Send data is too big");
        }

        std::string buffer = get_buffer(request_header{ protocol_len, body_len, flag }, protocol, body);
        async_write_impl(buffer);
    }



    void write_impl(const std::string& buffer)
    {
        boost::system::error_code ec;
        boost::asio::write(socket_, boost::asio::buffer(buffer), ec);
        if (ec)
        {
            is_connected_ = false;
            throw std::runtime_error(ec.message());
        }
    }

    void async_write_impl(const std::string& buffer)
    {
        ios_.post([this, buffer]
        {
            std::cout << "size: " << send_queue_.size() << std::endl;
            bool is_empty = send_queue_.empty();
            send_queue_.emplace_back(buffer);
            if (is_empty)
            {
                async_write_impl();
            }
        });
    }

    void async_write_impl()
    {
        boost::asio::async_write(socket_, boost::asio::buffer(send_queue_.front()), 
                                 [this](boost::system::error_code ec, std::size_t)
        {
            if (!ec)
            {
                send_queue_.pop_front();
                if (!send_queue_.empty())
                {
                    async_write_impl();
                }
            }
            else
            {
                is_connected_ = false;
                send_queue_.clear();
                log_warn(ec.message());
            }
        });
    }

    std::vector<char> read()
    {
        start_timer();
        auto guard = make_guard([this]{ stop_timer(); });
        read_head();
        check_head();
        return read_body();
    }

    void read_head()
    {
        boost::system::error_code ec;
        boost::asio::read(socket_, boost::asio::buffer(res_head_buf_), ec);
        if (ec)
        {
            is_connected_ = false;
            throw std::runtime_error(ec.message());
        }
    }

    void check_head()
    {
        memcpy(&res_head_, res_head_buf_, sizeof(res_head_buf_));
        if (res_head_.body_len > max_buffer_len)
        {
            throw std::runtime_error("Body len is too big");
        }
    }

    std::vector<char> read_body()
    {
        body_.clear();
        body_.resize(res_head_.body_len);
        boost::system::error_code ec;
        boost::asio::read(socket_, boost::asio::buffer(body_), ec); 
        if (ec)
        {
            is_connected_ = false;
            throw std::runtime_error(ec.message());
        }
        return body_;
    }

    void start_timer()
    {
        if (timeout_milli_ != 0)
        {
            timer_.start(timeout_milli_);
        }
    }

    void stop_timer()
    {
        if (timeout_milli_ != 0)
        {
            timer_.stop();
        }
    }

    void start_ios_thread()
    {
        thread_ = std::make_unique<std::thread>([this]{ ios_.run(); });
    }

    void start_timer_thread()
    {
        if (timeout_milli_ != 0)
        {
            timer_thread_ = std::make_unique<std::thread>([this]{ timer_ios_.run(); });
            timer_.bind([this]{ disconnect(); });
            timer_.set_single_shot(true);
        }
    }

    void start_heartbeats_thread()
    {
        if (client_type_ == client_type::sub_client)
        {
            heatbeats_thread_ = std::make_unique<std::thread>([this]{ heartbeats_ios_.run(); });
            heartbeats_timer_.bind([this]{ heartbeats_timer(); });
            heartbeats_timer_.start(heartbeats_milli);
        }
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

    void stop_timer_thread()
    {
        timer_ios_.stop();
        if (timer_thread_ != nullptr)
        {
            if (timer_thread_->joinable())
            {
                timer_thread_->join();
            }
        }
    }

    void stop_ios_thread()
    {
        ios_.stop();
        if (thread_ != nullptr)
        {
            if (thread_->joinable())
            {
                thread_->join();
            }
        }
    }

    void async_read_head()
    {
        boost::asio::async_read(socket_, boost::asio::buffer(push_head_buf_), 
                                [this](boost::system::error_code ec, std::size_t)
        {
            if (!socket_.is_open())
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
        boost::asio::async_read(socket_, boost::asio::buffer(protocol_and_body_), 
                                [this](boost::system::error_code ec, std::size_t)
        {
            async_read_head();

            if (!socket_.is_open())
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
            try_connect();
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

protected:
    client_type client_type_;

private:
    boost::asio::io_service ios_;
    boost::asio::io_service::work work_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver::iterator endpoint_iter_;
    std::unique_ptr<std::thread> thread_;
    char res_head_buf_[response_header_len];
    response_header res_head_;
    std::vector<char> body_;
    char push_head_buf_[push_header_len];
    push_header push_head_;
    std::vector<char> protocol_and_body_;

    boost::asio::io_service timer_ios_;
    boost::asio::io_service::work timer_work_;
    std::unique_ptr<std::thread> timer_thread_;
    atimer<> timer_;

    std::size_t timeout_milli_ = 0;
    std::atomic<bool> is_connected_ ;
    std::mutex mutex_;
    std::mutex conn_mutex_;

    boost::asio::io_service heartbeats_ios_;
    boost::asio::io_service::work heartbeats_work_;
    std::unique_ptr<std::thread> heatbeats_thread_;
    atimer<> heartbeats_timer_;

    async_send_queue send_queue_;
};

}

#endif
