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
#include <memory>
#include <map>
#include "tcpClient/define.h"
#include "tcpClient/eventServer.h"
#include "tcpClient/eventClient.h"
#include "logger/logger.hpp"

#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <thread>
#include <atomic>
#include <tuple>
class Loop : public std::enable_shared_from_this<Loop>
{
public:
	Loop();
	virtual ~Loop();

	inline event_base *get_event_base()
	{
		__LOG(debug, "[get event_base]: event base is : " << (void *)_base);
		return _base;
	}

	inline int status() const
	{
		return _status;
	}

	bool init(bool newThread);

	void stop(bool waiting = true);

	bool post_message();
	void process_message(uint64_t one);

protected:
	virtual bool onBeforeStart();

	virtual void onBeforeLoop();

	virtual void onAfterLoop();

	virtual void onAfterStop();

private:
	void _run();

private:
	event_base *_base;
	std::shared_ptr<std::thread> _thread_sptr;
	std::atomic<int> _status;
	TASK_QUEUE _task_queue;
	std::mutex mtx;
	std::shared_ptr<EventFdServer> _event_server;
	std::shared_ptr<EVFDClient> _event_client;
};
