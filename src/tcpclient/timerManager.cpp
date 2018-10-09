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
#include "tcpclient/timerManager.h"

namespace tcpclient
{

Timer::ptr_p TimerManager::getTimer(int *timerID)
{
	int tid = getUniqueID();
	Timer::ptr_p tmp_ptr(new Timer(_loop));
	{
		std::lock_guard<std::mutex> lck(mtx);
		//t_map.emplace(tid, tmp_ptr);
		t_map[tid] = tmp_ptr;
	}
	if (timerID)
	{
		*timerID = tid;
	}
	return tmp_ptr;
}
bool TimerManager::auditTimer()
{
	{
		std::lock_guard<std::mutex> lck(mtx);
		for (auto i = t_map.begin(); i != t_map.end();)
		{
			__LOG(debug, "[audit] now process timer with ID : " << i->first);
			if (i->second)
			{
				//if ((i->second)->isFinished())
				if (i->second)
				{
					if ((i->second).unique())
					{
						i = t_map.erase(i);
					}
					else
					{
						i++;
					}
				}
				else
				{
					i++;
				}
			}
			else
			{
				__LOG(warn, "timer ptr is invalid");
				i = t_map.erase(i);
			}
		}
		__LOG(debug, "audit done"
						 << ". There are " << t_map.size() << " timers left");
	}

	return true;
}
// you can call timer.stop or pass in the ID and we will kill the timer
bool TimerManager::killTimer(int timerID)
{
	//__LOG(debug, "before killing the timer " << timerID << ". There are " << t_map.size() << " timer");
	{
		std::lock_guard<std::mutex> lck(mtx);
		t_map.erase(timerID);
	}
	//__LOG(debug, "kill timer with ID : " << timerID << ". Now there are " << t_map.size() << " timer");
	return true;
}

} /* namespace tcpclient */
