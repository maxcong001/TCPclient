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

#pragma once
#include "tcpClient/loop.h"
#include "tcpClient/define.h"
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "logger/logger.hpp"

#define INPUT_BUFFER bufferevent_get_input(_bev)
#define OUTPUT_BUFFER bufferevent_get_output(_bev)

typedef evutil_socket_t SocketFd;
const SocketFd SOCKET_FD_INVALID = -1;

class tcpClient : public std::enable_shared_from_this<tcpClient>

{
  public:
	tcpClient(std::shared_ptr<Loop> loop);
	virtual ~tcpClient()
	{
		__LOG(warn, "dtor of tcpClient is called, this is : " << (void *)this);
	}

	bool init();

	inline bool isConnected() const
	{
		return _isConnected; //.load(std::memory_order_acquire 	);
	}

	void set_conn_info(CONN_INFO info)
	{
		_conn_info = info;
	}
	CONN_INFO get_conn_info()
	{
		return _conn_info;
	}
	bool disconnect();
	bool reconnect();
	bool connect(CONN_INFO _info);
	bool post_connect(CONN_INFO _info);
	bool set_source_addr(std::string source_addr, int fd = -1);
	uint32_t getInputBufferLength() const;
	const uint8_t *viewInputBuffer(uint32_t size) const;
	bool readInputBuffer(uint8_t *dest, uint32_t size);
	void clearInputBuffer();
	bool drainInputBuffer(uint32_t len);
	SocketFd get_socket() const;
	void getAddr(struct sockaddr_in *dest, uint32_t size) const;
	bool send(const char *data, uint32_t size);
	std::shared_ptr<Loop> get_loop()
	{
		return _loop;
	}

  protected:
	virtual void onRead()
	{
		__LOG(warn, "tcp client receive message");
	}
	virtual void onDisconnected()
	{

		__LOG(warn, "disconnect!!!!!");
	}
	virtual void onConnected(int error)
	{
		__LOG(warn, "connected!!!!!");
	};

  private:
	void handleEvent(short events);
	int set_sockopt_tos(unsigned int dscp, int _fd = -1);
	bool _connect(struct sockaddr *addr, unsigned int addr_len, int fd = -1);

	static void readCallback(struct bufferevent *bev, void *ctx);
	static void writeCallback(struct bufferevent *bev, void *ctx);
	static void eventCallback(struct bufferevent *bev, short events, void *ctx);

  private:
	std::shared_ptr<Loop> _loop;
	std::atomic<bool> _isConnected;
	CONN_INFO _conn_info;
	struct bufferevent *_bev;
};
