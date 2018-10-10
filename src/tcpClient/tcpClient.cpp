/*
 * Copyright (c) 2016-20017 Max Cong <savagecm@qq.com>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tcpClient/tcpClient.h"
#include <netinet/tcp.h>
#include "logger/logger.hpp"
#include <memory>





tcpClient::tcpClient(std::shared_ptr<Loop> loop) : _loop(loop),
												   _isConnected(false), _bev(NULL)
{

}

void tcpClient::handleEvent(short events)
{
	__LOG(warn, " handle event, error code is : " << events << ". error message is : " << evutil_socket_error_to_string(events) << ". this is : " << (void *)this);

	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		_isConnected = false; //.store(false, std::memory_order_release);
		onDisconnected();
		if (_bev)
		{
			bufferevent_free(_bev);
		}
		_bev = NULL;
		return;
	}

	if (events & BEV_EVENT_CONNECTED)
	{
		int socketError = EVUTIL_SOCKET_ERROR();
		__LOG(debug, "socketError is : " << socketError);
		if ((socketError != ECONNREFUSED) && (socketError != ETIMEDOUT))
		{
			_isConnected = true; //.store(true, std::memory_order_release);
			onConnected(socketError);
		}
		else
		{
			onConnected(socketError);
		}
	}
}

int tcpClient::set_sockopt_tos(unsigned int dscp, int _fd)
{
	if (!dscp)
	{
		__LOG(debug, "dscp is 0, no need to set it");
		return 0;
	}
	__LOG(debug, "[set_socket_option] fd is :" << _fd);
	if (!this->_bev)
	{
		return -1;
	}

	int ret;

	struct sockaddr_storage ss;
	socklen_t sslen = sizeof(ss);
	evutil_socket_t fd = _fd;
	if (fd < 0)
	{
		fd = get_socket();
	}

	if (fd < 0)
	{
		__LOG(error, "invalid fd!");
		return -1;
	}

	ret = getsockname(fd, (struct sockaddr *)&ss, &sslen);
	if (ret != 0)
		return ret;

	unsigned char tos = (unsigned char)((dscp & 0x3f) << 2);

	switch (((struct sockaddr *)&ss)->sa_family)
	{
	case AF_INET:
		ret = setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
		__LOG(debug, "set socket option return : " << ret);
		break;
	case AF_INET6:
		ret = setsockopt(fd, IPPROTO_IPV6, IPV6_TCLASS, &tos, sizeof(tos));
		break;
	default:
		return -1;
	}
	return ret;
}
bool tcpClient::post_connect(CONN_INFO info)
{
	TASK_MSG msg;
	msg.type = TASK_MSG_TYPE::TASK_ADD_CONN;
	msg.body = std::make_tuple(info, shared_from_this());
	return _loop->post_message(msg);
}
bool tcpClient::connect(CONN_INFO _info)
{
	set_conn_info(_info);

	switch (_info.get_conn_type())
	{
	case HostType::IPV4:
	{
		int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (sock_fd < 0)
		{
			__LOG(error, " create a socket fd return fail");
			return false;
		}
		uint16_t port = 0;
		try
		{
			// note: convert int to uint16
			port = std::stoi(_info.get_port());
		}
		catch (std::exception &e)
		{
			__LOG(error, "!!!!!!!!!!!!exception happend when trying to cast port, info :" << e.what() << ". string is : " << _info.get_port());
			return false;
		}
		__LOG(debug, "now connect to " << _info.ip << ":" << _info.port << ", with socket : " << sock_fd);
		struct sockaddr_in serverAddr;
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		evutil_inet_pton(AF_INET, _info.get_ip().c_str(), &serverAddr.sin_addr);
		serverAddr.sin_port = htons(port);
		return _connect((struct sockaddr *)&serverAddr, sizeof(serverAddr), sock_fd);
	}
	break;
	case HostType::IPV6:
	{
		int sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
		if (sock_fd < 0)
		{
			__LOG(error, " create a socket fd return fail");
			return false;
		}

		uint16_t port = 0;
		try
		{
			// note: convert int to uint16
			port = std::stoi(_info.get_port());
		}
		catch (std::exception &e)
		{
			__LOG(error, "!!!!!!!!!!!!exception happend when trying to cast port, info :" << e.what());
			return false;
		}
		struct sockaddr_in6 serverAddr;
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin6_port = htons(port);
		serverAddr.sin6_family = AF_INET6;
		evutil_inet_pton(AF_INET6, _info.get_ip().c_str(), &serverAddr.sin6_addr);
		return _connect((struct sockaddr *)&serverAddr, sizeof(serverAddr), sock_fd);
	}
	break;
	case HostType::UNIX_SOCKET:
	{
		int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (sock_fd < 0)
		{
			__LOG(error, " create a socket fd return fail");
			return false;
		}

		struct sockaddr_un serverAddr;
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sun_family = AF_UNIX;
		strncpy(serverAddr.sun_path, _info.get_path().c_str(), _info.get_path().size());
		return _connect((struct sockaddr *)&serverAddr, sizeof(serverAddr), sock_fd);
	}
	break;
	default:
		__LOG(error, "not support type! type is : " << (int)_info.get_conn_type());
		return false;
		break;
	}
}
bool tcpClient::_connect(struct sockaddr *addr, unsigned int addr_len, int fd)
{
	evutil_make_socket_nonblocking((fd < 0) ? get_socket() : fd);
	// note: note no cross check for source IP
	set_source_addr(get_conn_info().get_source_ip(), ((fd < 0) ? get_socket() : fd));

	this->_bev = bufferevent_socket_new(_loop->get_event_base(), fd,
										BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
	if (NULL == this->_bev)
	{
		__LOG(error, "buffer event new return fail!");
		return false;
	}
	__LOG(debug, "[_connect] buffer_event is : " << (void *)(this->_bev) << ", this is : " << (void *)this);

	bufferevent_setcb(this->_bev, readCallback, writeCallback, eventCallback, this);
	if (-1 == bufferevent_enable(this->_bev, EV_READ | EV_WRITE))
	{
		bufferevent_free(this->_bev);
		this->_bev = NULL;
		return false;
	}
	__LOG(debug, "[_connect]buffer_event is : " << (void *)(this->_bev) << ", this is : " << (void *)this);
	if (-1 == bufferevent_socket_connect(this->_bev, (struct sockaddr *)addr, addr_len))
	{
		__LOG(warn, "beffer event connect return fail");
		bufferevent_free(this->_bev);
		this->_bev = NULL;
		return false;
	}

	int sockret = set_sockopt_tos(get_conn_info().get_dscp(), ((fd < 0) ? get_socket() : fd));
	if (sockret != 0)
	{
		__LOG(error, "Failed to set IP_TOS: DSCP" << get_conn_info().get_dscp() << ", fd= " << fd << ", ret" << sockret);
	}
	return true;
}

void tcpClient::readCallback(struct bufferevent *bev, void *ctx)
{
	tcpClient *socket = (tcpClient *)ctx;
	socket->onRead();
}

void tcpClient::writeCallback(struct bufferevent *bev, void *ctx)
{
}

void tcpClient::eventCallback(struct bufferevent *bev, short events, void *ctx)
{
	tcpClient *socket = (tcpClient *)ctx;
	socket->handleEvent(events);
}

uint32_t tcpClient::getInputBufferLength() const
{
	return (NULL != _bev) ? evbuffer_get_length(INPUT_BUFFER) : 0;
}

const uint8_t *tcpClient::viewInputBuffer(uint32_t size) const
{
	return (NULL != _bev) ? evbuffer_pullup(INPUT_BUFFER, size) : NULL;
}

bool tcpClient::readInputBuffer(uint8_t *dest, uint32_t size)
{
	return (NULL != _bev) ? (-1 != evbuffer_remove(INPUT_BUFFER, (void *)dest, size)) : false;
}

void tcpClient::clearInputBuffer()
{
	if (NULL != _bev)
	{
		evbuffer_drain(INPUT_BUFFER, UINT32_MAX);
	}
}

bool tcpClient::drainInputBuffer(uint32_t len)
{
	return (NULL != _bev) ? (-1 != evbuffer_drain(INPUT_BUFFER, len)) : false;
}

SocketFd tcpClient::get_socket() const
{
	return (NULL == _bev) ? SOCKET_FD_INVALID : bufferevent_getfd(_bev);
}

void tcpClient::getAddr(struct sockaddr_in *dest, uint32_t size) const
{
	uint32_t addrSize = sizeof(struct sockaddr_in);
	if ((NULL == _bev) || (size < addrSize))
	{
		return;
	}
	getpeername(get_socket(), (struct sockaddr *)dest, &addrSize);
}

bool tcpClient::send(const char *data, uint32_t size)
{
	if (NULL == _bev)
	{
		return false;
	}
	return (-1 != bufferevent_write(_bev, data, size));
}

bool tcpClient::set_source_addr(std::string source_addr, int fd)
{
	if (source_addr.empty())
	{
		__LOG(debug, "no source IP, no need to set ");
		return true;
	}
	evutil_socket_t s;
	if (fd < 0)
	{
		s = get_socket();
	}
	else
	{
		s = fd;
	}

	if (s < 0)
	{
		__LOG(error, "invalid socket fd!");
		return false;
	}
	struct addrinfo *bservinfo, *b; //, hints ;
	if (!source_addr.empty())
	{
		int rv = -1;
		/* Using getaddrinfo saves us from self-determining IPv4 vs IPv6 */
		if ((rv = getaddrinfo(source_addr.c_str(), NULL, NULL, &bservinfo)) != 0) //&hints, &bservinfo)) != 0)
		{
			__LOG(error, "get addr info return fail!");
			// fail, need to call freeaddrinfo? Seems not needed
			//freeaddrinfo(bservinfo);
			return false;
		}
		for (b = bservinfo; b != NULL; b = b->ai_next)
		{
			if (bind(s, b->ai_addr, b->ai_addrlen) != -1)
			{
				__LOG(error, "bind faill!!!");
				break;
			}
		}
	}
	else
	{
		return false;
	}
	freeaddrinfo(bservinfo);
	return true;
}
void tcpClient::reconnect()
{
	if (this->_bev)
	{
		bufferevent_free(this->_bev);
	}
	_isConnected = false;
	connect(get_conn_info());
}

bool tcpClient::init()
{
	__LOG(debug, "[tcpClient::init], this is : " << (void *)this);
	if (NULL != this->_bev)
	{
		__LOG(warn, "buffer event is not NULL");
		bufferevent_free(this->_bev);
		this->_bev = NULL;
	}        



	return true;
}
