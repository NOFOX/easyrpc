#ifndef _HEADER_H
#define _HEADER_H

#include <string>

namespace easyrpc
{

const int max_buffer_len = 8192;
const int request_header_len = 16;
const int response_header_len = 4;

enum class call_mode : unsigned int
{
    raw,
    non_raw
};

enum class client_type : unsigned int
{
    rpc_client,
    pub_client,
    sub_client
};

struct client_flag
{
    call_mode mode;
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

using one_way = void;
using two_way = std::string;

}

#endif
