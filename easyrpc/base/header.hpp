#ifndef _HEADER_H
#define _HEADER_H

#include <string>

namespace easyrpc
{

constexpr const int max_buffer_len = 8 * 1024 * 1024;
const int request_header_len = 16;
const int response_header_len = 4;
const int push_header_len = 12;
const std::string subscribe_topic_flag = "1";
const std::string cancel_subscribe_topic_flag = "0";

enum class serialize_mode : unsigned int
{
    serialize,
    non_serialize
};

enum class client_type : unsigned int
{
    rpc_client,
    pub_client,
    sub_client
};

struct client_flag
{
    serialize_mode mode;
    client_type type;
};

struct request_header
{
    unsigned int protocol_len;
    unsigned int body_len;
    client_flag flag;
};

struct response_header
{
    unsigned int body_len;
};

struct push_header
{
    unsigned int protocol_len;
    unsigned int body_len;
    serialize_mode mode;
};

struct endpoint
{
    std::string ip;
    unsigned short port;
};

using one_way = void;
using two_way = std::string;

}

#endif
