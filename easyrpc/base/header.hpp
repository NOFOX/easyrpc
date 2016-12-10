#ifndef _HEADER_H
#define _HEADER_H

#include <string>

namespace easyrpc
{

const int max_buffer_len = 8192;
const int request_header_len = 16;
const int response_header_len = 4;
const std::string subscribe_topic_flag = "true";
const std::string cancel_subscribe_topic_flag = "false";

enum class serialize_mode : unsigned int
{
    serialize,
    non_serialize
};

enum class call_mode : unsigned int
{
    rpc_mode,
    pub_mode,
    sub_mode
};

struct client_flag
{
    serialize_mode s_mode;
    call_mode c_mode;
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

struct endpoint
{
    std::string ip;
    unsigned short port;
};

using one_way = void;
using two_way = std::string;

}

#endif
