/**
 * @file IOThread.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <bbt/base/assert/Assert.hpp>
#include "bbt/network/adapter/libevent/IOThread.hpp"
#include "bbt/network/adapter/libevent/EventLoop.hpp"

namespace bbt::network::libevent
{

IOThread::IOThread()
    :m_eventloop(std::make_unique<EventLoop>())
{
    Init();
}

IOThread::IOThread(const HookCallback& thread_begin, const HookCallback& thread_end)
    :m_eventloop(std::make_unique<EventLoop>())
{
    SetOnThreadBegin_Hook(thread_begin);
    SetOnThreadEnd_Hook(thread_end);
}


IOThread::~IOThread()
{
    /* 等待线程停止 */
    if(m_status == IOThreadRunStatus::Running)
        Stop();
    /* 资源回收 */
    Destory();
}

void IOThread::Init()
{
    SetOnThreadBegin_Hook([](IOThreadID tid){});
    SetOnThreadEnd_Hook([](IOThreadID tid){});
}

void IOThread::Destory()
{
    m_eventloop = nullptr;
}


void IOThread::evWorkFunc()
{
}

void IOThread::WorkHandle()
{
    auto err = m_eventloop->StartLoop(EVLOOP_NO_EXIT_ON_EMPTY);
    Assert(err);
}

const std::unique_ptr<EventLoop>& IOThread::GetEventLoop() const
{
    return m_eventloop;
}

Errcode IOThread::Stop()
{
    if (m_status == Finish)
        return FASTERR_NOTHING;

    auto err = m_eventloop->BreakLoop();
    if (!err)
        return err;
    
    /* 阻塞式的等待 */
    SyncWaitThreadExit();

    return err;
}

std::shared_ptr<Event> IOThread::RegisterEvent(evutil_socket_t fd, short events, const OnEventCallback& onevent_cb)
{
    auto event_sptr = m_eventloop->CreateEvent(fd, events, onevent_cb);
    return event_sptr;
}

}// namespace end