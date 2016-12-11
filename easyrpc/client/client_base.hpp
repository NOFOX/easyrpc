#ifndef _CLIENT_BASE_H
#define _CLIENT_BASE_H

#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <boost/asio.hpp>
#include "base/header.hpp"
#include "base/atimer.hpp"
#include "base/scope_guard.hpp"
#include "base/logger.hpp"

namespace easyrpc
{

class client_base
{
public:
    client_base() : work_(ios_), socket_(ios_), 
    timer_work_(timer_ios_), timer_(timer_ios_) {}
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
        thread_ = std::make_unique<std::thread>([this]{ ios_.run(); });
        if (timeout_milli_ != 0)
        {
            timer_thread_ = std::make_unique<std::thread>([this]{ timer_ios_.run(); });
        }
        try_connect();
    }

    void stop()
    {
        stop_ios_thread();
        stop_timer_thread();
    }

    void call_one_way(const std::string& protocol, const client_flag& flag, const std::string& body)
    {
        try_connect();
        write(protocol, flag, body);
    }

    std::vector<char> call_two_way(const std::string& protocol, const client_flag& flag, const std::string& body)
    {
        call_one_way(protocol, flag, body);
        return read();
    }

    void do_read()
    {
        async_read_head();
    }

    void connect()
    {
        boost::asio::connect(socket_, endpoint_iter_);
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

private:
    void write(const std::string& protocol, const client_flag& flag, const std::string& body)
    {
        unsigned int protocol_len = static_cast<unsigned int>(protocol.size());
        unsigned int body_len = static_cast<unsigned int>(body.size());
        if (protocol_len + body_len > max_buffer_len)
        {
            throw std::runtime_error("Send data is too big");
        }

        const auto& buffer = get_buffer(request_header{ protocol_len, body_len, flag }, protocol, body);
        write_impl(buffer);
    }

    std::vector<boost::asio::const_buffer> get_buffer(const request_header& head, 
                                                      const std::string& protocol, 
                                                      const std::string& body)
    {
        std::vector<boost::asio::const_buffer> buffer;
        buffer.emplace_back(boost::asio::buffer(&head, sizeof(request_header)));
        buffer.emplace_back(boost::asio::buffer(protocol));
        buffer.emplace_back(boost::asio::buffer(body));
        return buffer;
    }

    void write_impl(const std::vector<boost::asio::const_buffer>& buffer)
    {
        boost::system::error_code ec;
        boost::asio::write(socket_, buffer, ec);
        if (ec)
        {
            is_connected_ = false;
            throw std::runtime_error(ec.message());
        }
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

    void try_connect()
    {
        if (!is_connected_)
        {
            connect();
            is_connected_ = true;
        }
    }

    void start_timer()
    {
        if (timeout_milli_ == 0)
        {
            return;
        }

        timer_.bind([this]{ disconnect(); });
        timer_.set_single_shot(true);
        timer_.start(timeout_milli_);
    }

    void stop_timer()
    {
        if (timeout_milli_ == 0)
        {
            return;
        }
        timer_.stop();
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

            bool ok = route(std::string(&protocol_and_body_[0], push_head_.protocol_len), 
                             std::string(&protocol_and_body_[push_head_.protocol_len], push_head_.body_len));
            if (!ok)
            {
                log_warn("Router failed");
                return;
            }
        });
    }

    bool route(const std::string& protocol, const std::string& body)
    {
        std::cout << "protocol: " << protocol << std::endl;
        std::cout << "body: " << body << std::endl;
        return true;
    }

protected:
    client_type type_;

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
    bool is_connected_ = false;
};

}

#endif
