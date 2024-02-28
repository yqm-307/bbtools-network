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
}


void IOThread::evWorkFunc()
{
}

void IOThread::WorkHandle()
{
    m_eventloop->StartLoop(EventLoopOpt::LOOP_NO_EXIT_ON_EMPTY);
}

const std::unique_ptr<EventLoop>& IOThread::GetEventLoop() const
{
    return m_eventloop;
}

Errcode IOThread::Stop()
{
    auto err = m_eventloop->BreakLoop();
    
    /* 阻塞式的等待 */
    SyncWaitThreadExit();
    return err;
}

std::shared_ptr<Event> IOThread::RegisterEvent(evutil_socket_t fd, short events, const OnEventCallback& onevent_cb)
{
    auto event_sptr = m_eventloop->CreateEvent(fd, events, onevent_cb);
    return event_sptr;
}

std::shared_ptr<Event> IOThread::RegisterEventSafe(evutil_socket_t fd, short events, const OnEventCallback& onevent_cb)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto event_sptr = RegisterEvent(fd, events, onevent_cb);
    return event_sptr;
}

}// namespace end