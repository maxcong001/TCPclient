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

#include "tcpClient/loop.h"
#include "tcpClient/tcpClient.h"
#include "tcpClient/eventClient.h"
#include "tcpClient/eventServer.h"
void evfdCallback(int fd, short event, void *args)
{
    uint64_t one;
    int ret = read(fd, &one, sizeof one);
    if (ret != sizeof one)
    {
        __LOG(warn, "read return : " << ret);
        return;
    }
    Loop *tmp = reinterpret_cast<Loop *>(args);
    tmp->process_message(one);
}

bool Loop::post_message(TASK_MSG msg)
{
    {
        std::lock_guard<std::mutex> lck(mtx);
        _task_queue.push(msg);
    }
    return _event_client->send();
}
void Loop::process_message(uint64_t one)
{
    TASK_QUEUE _tmp_task_queue;
    {
        std::lock_guard<std::mutex> lck(mtx);
        // actually process all the messages
        swap(_task_queue, _tmp_task_queue);
        if (_tmp_task_queue.empty())
        {
            __LOG(warn, "_tmp_task_queue queue is empty");
            return;
        }
    }
    while (_tmp_task_queue.size() != 0)
    {
        auto tmp = _tmp_task_queue.front();
        switch (tmp.type)
        {
        case TASK_MSG_TYPE::TASK_ADD_CONN:
        {
            std::tuple<CONN_INFO, std::shared_ptr<tcpClient>> msg_body;
            try
            {
                msg_body = TASK_ANY_CAST<std::tuple<CONN_INFO, std::shared_ptr<tcpClient>>>(tmp.body);
            }
            catch (std::exception &e)
            {
                __LOG(error, "!!!!!!!!!!!!exception happend when trying to cast message, info :" << e.what());
                return;
            }
            std::get<1>(msg_body)->connect(std::get<0>(msg_body));
        }
        break;
        case TASK_MSG_TYPE::TASK_STOP_LOOP_THREAD:
        {
            this->stop();

            __LOG(warn, "receive a stop loop thread message!");
        }
        break;
        default:
            __LOG(error, "unsupport message type");
            break;
        }
        _tmp_task_queue.pop();
    }
}
Loop::Loop() : _base(NULL),
               _status(StatusInit)
{
}

Loop::~Loop()
{
    TASK_MSG msg;
    msg.type = TASK_MSG_TYPE::TASK_STOP_LOOP_THREAD;
    post_message(msg);
    if (_thread_sptr)
    {
        _thread_sptr->join();
    }
    if (NULL != _base)
    {
        event_base_free(_base);
        _base = NULL;
    }
}

bool Loop::init(bool newThread)
{
    evthread_use_pthreads();
    _base = event_base_new();
    if (!_base)
    {
        throw std::logic_error(CREATE_EVENT_FAIL);
    }

    int _evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (_evfd <= 0)
    {
        __LOG(error, "!!!!!!!!create event fd fail!");
        return false;
    }

    try
    {
        _event_server = std::make_shared<EventFdServer>(get_event_base(), _evfd, evfdCallback, this);
    }
    catch (std::exception &e)
    {
        __LOG(error, "!!!!!!!!!!!!exception happend when trying to create event fd server, info :" << e.what());
        return false;
    }

    _event_client = std::make_shared<EventFdClient>(_evfd);

    if (!onBeforeStart())
    {
        return false;
    }
    if (newThread)
    {
        _thread_sptr.reset(new std::thread(std::bind(&Loop::_run, this)));
    }
    else
    {
        _run();
    }
    return true;
}

void Loop::_run()
{
    _status = StatusRunning;
    onBeforeLoop();
    event_base_loop(_base, 0);
    __LOG(debug, "base loop is exiting...");
    onAfterLoop();
    _status = StatusFinished;
}
void Loop::stop(bool waiting)
{
    if (StatusFinished == _status)
    {
        return;
    }
    //if call the active callback before exit
    waiting ? event_base_loopexit(_base, NULL) : event_base_loopbreak(_base);
    onAfterStop();
}

bool Loop::onBeforeStart()
{
    return true;
}

void Loop::onBeforeLoop()
{
}

void Loop::onAfterLoop()
{
}

void Loop::onAfterStop()
{
}
