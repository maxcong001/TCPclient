#pragma once

#include "logger/logger.hpp"
#include "boost/asio/ip/address.hpp"
#include <queue>

#if __cplusplus >= 201703L
// use std::any
#include <any>
#define TASK_ANY std::any
#define TASK_ANY_CAST std::any_cast
#else
#include <boost/any.hpp>
#define TASK_ANY boost::any
#define TASK_ANY_CAST boost::any_cast
#endif

enum class TASK_MSG_TYPE : std::uint32_t
{
    TASK_ADD_CONN,
    TASK_STOP_LOOP_THREAD,
    TASK_MAX
};
struct TASK_MSG
{
    TASK_MSG_TYPE type;
    std::uint32_t seq_id;
    std::string from;
    std::string to;
    TASK_ANY body;
};
typedef std::queue<TASK_MSG> TASK_QUEUE;

using boost::asio::ip::address;

#define CREATE_EVENT_FAIL "CREATE_EVENT_FAIL"

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
        UNIX_SOCKET,
        MAX
    };

    static std::string toString(const HostType::type _type)
    {
        return (_type >= MAX || _type < IPV4) ? "UNDEFINED_HOSTTYPE" : (const std::string[]){"   IPV4", "   IPV6", "   FQDN"}[_type];
    }
};
static HostType::type getHostType(const std::string &host)
{
    if (host.empty())
    {
        __LOG(error, "host is empty");
        return HostType::MAX;
    }
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
            return HostType::UNIX_SOCKET;
        }
    }
    /*
    catch (boost::system::system_error &e)
    {
        //Expected exception for FQDN
        __LOG(debug, "got error while get host type, host is : " << host);
        return HostType::MAX;
    }
    */
    catch (...)
    {
        //Expected exception for FQDN
        __LOG(debug, "got error while get host type, host is : " << host);
        return HostType::MAX;
    }
}

struct CONN_INFO
{
    std::string ip;
    std::string port;
    std::string source_ip;
    std::string source_port;
    std::string path;
    unsigned int dscp;

    CONN_INFO()
    {
        dscp = 0;
    }

    HostType::type get_conn_type()
    {
        return getHostType(ip);
    }

    std::tuple<std::string, std::string, std::string, std::string, bool> get_ip_port()
    {
        bool isV6 = ((get_conn_type() == HostType::IPV6) ? true : false);
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
    inline bool operator<(const  CONN_INFO &rhs)
    {
        return true;
    }
    */
    inline CONN_INFO &operator=(CONN_INFO rhs)
    {
        ip = rhs.ip;
        port = rhs.port;
        source_ip = rhs.source_ip;
        source_port = rhs.source_port;
        path = rhs.path;
        dscp = rhs.dscp;
        return *this;
    }
    inline bool operator==(CONN_INFO rhs)
    {
        return (source_port == rhs.source_port) && (dscp == rhs.dscp) && (!ip.compare(rhs.get_ip())) && (!port.compare(rhs.get_port())) && (!source_ip.compare(rhs.get_source_ip())) && (!path.compare(rhs.get_path()));
    }
};
