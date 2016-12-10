#ifndef _TCP_ENDPOINT_H
#define _TCP_ENDPOINT_H

namespace easyrpc
{

#include "io_service_pool.hpp"
#include "connection.hpp"

class tcp_endpoint
{
public:
    tcp_endpoint(const router_callback& route_func, 
                 const remove_all_topic_callback& remove_all_topic_func) 
        : acceptor_(io_service_pool::singleton::get()->get_io_service()),
        route_(route_func), 
        remove_all_topic_(remove_all_topic_func) {}

    void listen(const std::string& ip, unsigned short port)
    {
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string(ip), port);
        acceptor_.open(ep.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen();
    }

    void accept()
    {
        auto new_conn = std::make_shared<connection>(io_service_pool::singleton::get()->get_io_service(), 2000, route_, remove_all_topic_);
        acceptor_.async_accept(new_conn->socket(), [this, new_conn](boost::system::error_code ec)
        {
            if (!ec)
            {
                new_conn->start();
            }
            accept();
        });
    }

    bool route(const std::string& protocol, const std::string& body,
               const client_flag& flag, const connection_ptr& conn)
    {
        return route_(protocol, body, flag, conn); 
    }

    void remove_all_topic(const connection_ptr& conn)
    {
        remove_all_topic_(conn);
    }

private:
    boost::asio::ip::tcp::acceptor acceptor_;
    router_callback route_;
    remove_all_topic_callback remove_all_topic_;
};

}

#endif
