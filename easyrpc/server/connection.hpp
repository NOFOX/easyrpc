#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include <boost/timer.hpp>
#include "base/header.hpp"
#include "base/atimer.hpp"
#include "base/scope_guard.hpp"
#include "base/logger.hpp"

namespace easyrpc
{

class connection;
using connection_ptr = std::shared_ptr<connection>;
using connection_weak_ptr = std::weak_ptr<connection>;

using router_callback = std::function<bool(const std::string&, const std::string&, 
                                           const client_flag&, const std::shared_ptr<connection>&)>;
using remove_all_topic_callback = std::function<void(const connection_ptr&)>;

class connection : public std::enable_shared_from_this<connection>
{
public:
    connection() = default;
    connection(const connection&) = delete;
    connection& operator=(const connection&) = delete;
    connection(boost::asio::io_service& ios, 
               std::size_t timeout_milli, const router_callback& route_func, 
               const remove_all_topic_callback& remove_all_topic_func)
        : socket_(ios), timer_(ios), 
        timeout_milli_(timeout_milli), route_(route_func), 
        remove_all_topic_(remove_all_topic_func) {} 

    ~connection()
    {
        disconnect();
    }

    void start()
    {
        set_no_delay();
        read_head();
    }

    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    void write(const std::string& body)
    {
        unsigned int body_len = static_cast<unsigned int>(body.size());
        if (body_len > max_buffer_len)
        {
            try_remove_all_topic();
            throw std::runtime_error("Send data is too big");
        }

        const auto& buffer = get_buffer(response_header{ body_len }, body);
        write_impl(buffer);
    }

    void write(const std::string& protocol, const std::string& body, serialize_mode mode)
    {
        unsigned int protocol_len = static_cast<unsigned int>(protocol.size());
        unsigned int body_len = static_cast<unsigned int>(body.size());
        if (protocol_len + body_len > max_buffer_len)
        {
            try_remove_all_topic();
            throw std::runtime_error("Send data is too big");
        }

        const auto& buffer = get_buffer(push_header{ protocol_len,  body_len, mode }, protocol, body);
        write_impl(buffer);
    }

    void disconnect()
    {
        if (socket_.is_open())
        {
            boost::system::error_code ignore_ec;
            socket_.shutdown(boost::asio::socket_base::shutdown_both, ignore_ec);
            socket_.close(ignore_ec);
        }
    }

private:
    void read_head()
    {
        auto self(this->shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(req_head_buf_), 
                                [this, self](boost::system::error_code ec, std::size_t)
        {
            auto guard = make_guard([this, self]{ try_remove_all_topic(); });
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

            if (check_head())
            {
                read_protocol_and_body();
                guard.dismiss();
            }
        });
    }

    bool check_head()
    {
        memcpy(&req_head_, req_head_buf_, sizeof(req_head_buf_));
        unsigned int len = req_head_.protocol_len + req_head_.body_len;
        return (len > 0 && len < max_buffer_len) ? true : false;
    }

    void read_protocol_and_body()
    {
        protocol_and_body_.clear();
        protocol_and_body_.resize(req_head_.protocol_len + req_head_.body_len);
        auto self(this->shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(protocol_and_body_), 
                                [this, self](boost::system::error_code ec, std::size_t)
        {
            read_head();
            auto guard = make_guard([this, self]{ try_remove_all_topic(); });
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

            bool ok = route_(std::string(&protocol_and_body_[0], req_head_.protocol_len), 
                             std::string(&protocol_and_body_[req_head_.protocol_len], req_head_.body_len), 
                             req_head_.flag, self);
            if (!ok)
            {
                log_warn("Router failed");
                return;
            }
            guard.dismiss();
        });
    }

    void set_no_delay()
    {
        boost::asio::ip::tcp::no_delay option(true);
        boost::system::error_code ec;
        socket_.set_option(option, ec);
    }

    void start_timer()
    {
        if (timeout_milli_ == 0)
        {
            return;
        }

        auto self(this->shared_from_this());
        timer_.bind([this, self]{ disconnect(); });
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

    std::vector<boost::asio::const_buffer> get_buffer(const response_header& head, const std::string& body)
    {
        std::vector<boost::asio::const_buffer> buffer;
        buffer.emplace_back(boost::asio::buffer(&head, sizeof(response_header)));
        buffer.emplace_back(boost::asio::buffer(body));
        return buffer;
    }

    std::vector<boost::asio::const_buffer> get_buffer(const push_header& head, const std::string& protocol, const std::string& body)
    {
        std::vector<boost::asio::const_buffer> buffer;
        buffer.emplace_back(boost::asio::buffer(&head, sizeof(push_header)));
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
            try_remove_all_topic();
            throw std::runtime_error(ec.message());
        }
    }

    void try_remove_all_topic()
    {
        if (req_head_.flag.type == client_type::sub_client)
        {
            remove_all_topic_(this->shared_from_this());
        }
    }

private:
    boost::asio::ip::tcp::socket socket_;
    char req_head_buf_[request_header_len];
    request_header req_head_;
    std::vector<char> protocol_and_body_;
    atimer<> timer_;
    std::size_t timeout_milli_ = 0;
    router_callback route_;
    remove_all_topic_callback remove_all_topic_;
};

}

#endif
