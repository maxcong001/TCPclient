#pragma once

#include "logger/logger.hpp"
#define CREATE_EVENT_FAIL "CREATE_EVENT_FAIL"
namespace tcpclient
{
typedef evutil_socket_t SocketFd;
const SocketFd SOCKET_FD_INVALID = -1;

enum CONN_STATUS
{
    StatusInit = 0,
    StatusRunning,
    StatusFinished
};

struct HostType
{
    enum type
    {
        IPV4,
        IPV6,
        FQDN,
        MAX
    };

    static std::string toString(const HostType::type _type)
    {
        return (_type >= MAX || _type < IPV4) ? "UNDEFINED_HOSTTYPE" : (const std::string[]){"   IPV4", "   IPV6", "   FQDN"}[_type];
    }
};
static HostType::type getHostType(const std::string &host)
{
    try
    {
        address addr(address::from_string(host));
        if (addr.is_v4())
        {
            return HostType::IPV4;
        }
        else if (addr.is_v6())
        {
            return HostType::IPV6;
        }
        else
        {
            return HostType::FQDN;
        }
    }
    catch (boost::system::system_error &e)
    {
        //Expected exception for FQDN
        logger->debug(DBW_LOG,
                      "Got error(%s) when parsing host %s, treat as FQDN.\n",
                      e.what(), host.c_str());
        return HostType::FQDN;
    }
}

struct conn_info
{
    std::string ip;
    std::string port;
    std::string source_ip;
    std::string source_port;
    std::string path;
    unsigned int dscp;

    conn_info()
    {
        type = HostType::MAX;
        dscp = 0;
    }

    HostType::type get_conn_type()
    {
        return getHostType(ip);
    }

    std::tuple<std::string, std::string, std::string, std::string, bool> get_ip_port()
    {

        bool isV6 = ((type == conn_type::IPv6) ? true : false);
        return std::make_tuple(ip, port, source_ip, source_port, isV6);
    }
    std::string get_ip()
    {
        return ip;
    }
    std::string get_port()
    {
        return port;
    }
    std::string get_source_ip()
    {
        return source_ip;
    }
    std::string get_path()
    {
        return path;
    }
    unsigned int get_dscp()
    {
        return dscp;
    }
    /*
    inline bool operator<(const conn_info &rhs)
    {
        return true;
    }
    */
    inline conn_info &operator=(conn_info rhs)
    {
        type = rhs.type;
        ip = rhs.ip;
        port = rhs.port;
        source_ip = rhs.source_ip;
        source_port = rhs.source_port;
        path = rhs.path;
        dscp = rhs.dscp;
        return *this;
    }
    inline bool operator==(conn_info rhs)
    {
        return (source_port == rhs.source_port) && (dscp == rhs.dscp) && ((type == rhs.type) && (!ip.compare(rhs.get_ip())) && (!port.compare(rhs.get_port())) && (!source_ip.compare(rhs.get_source_ip())) && (!path.compare(rhs.get_path())));
    }
};
}; // namespace tcpclient