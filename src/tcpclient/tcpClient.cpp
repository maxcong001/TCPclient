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

#include "tcpclient/tcpClient.h"
#include <netinet/tcp.h>
#include "logger/logger.hpp"
namespace tcpclient
{

TcpClient::TcpClient(std::shared_ptr<tcpclient::Loop> loop) : _loop(loop),
															 _isConnected(false)
{
	_dscp = 0;
}

void TcpClient::handleEvent(short events)
{
	__LOG(warn, " handle event, error code is : " << events << ". error message is : " << evutil_socket_error_to_string(events) << ". this is : " << (void *)this);

	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		bufferevent_disable(_bev, EV_READ | EV_WRITE);
		evutil_closesocket(get_socket());
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

void TcpClient::readCallback(struct bufferevent *bev, void *ctx)
{
	TcpClient *socket = (TcpClient *)ctx;
	socket->onRead();
}

void TcpClient::writeCallback(struct bufferevent *bev, void *ctx)
{
	TcpClient *socket = (TcpClient *)ctx;
	socket->checkClosing();
}

void TcpClient::eventCallback(struct bufferevent *bev, short events, void *ctx)
{
	TcpClient *socket = (TcpClient *)ctx;
	socket->handleEvent(events);
}

} /* namespace tcpclient */
