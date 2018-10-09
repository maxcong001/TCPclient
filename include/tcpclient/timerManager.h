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

#include "loop.h"
#include "timer.h"
#include <unordered_map>
#include <map>
#include <atomic>
#include <thread>
#include <algorithm>
#include "logger/logger.hpp"
namespace tcpclient
{
class Loop;
class Timer;
#define AUDIT_TIMER 5000
class TimerManager
{
  public:
	TimerManager(std::shared_ptr<Loop> loop_in) : uniqueID_atomic(0), t_map(), _loop(loop_in)
	{
	}
	~TimerManager()
	{
	}
	bool init()
	{
		getTimer()->startForever(AUDIT_TIMER, [this] {
			auditTimer();
		});
		return true;
	}
	bool stop()
	{
		std::lock_guard<std::mutex> lck(mtx);
		t_map.clear();
		return true;
	}

	Timer::ptr_p getTimer(int *timerID = NULL);
	bool killTimer(int timerID);
	bool auditTimer();

	std::mutex mtx;

  protected:
  private:
	int getUniqueID()
	{
		return (uniqueID_atomic++);
	}

	std::atomic<int> uniqueID_atomic;
	std::map<int, Timer::ptr_p> t_map;
	std::shared_ptr<Loop> _loop;
};

} /* namespace tcpclient */
