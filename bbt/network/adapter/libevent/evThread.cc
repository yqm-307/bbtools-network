#include "./evThread.hpp"

namespace bbt::network::libevent
{

void evThread::Start()
{
    m_thread = new std::thread([this](){Work();});
}

void evThread::Stop()
{
    m_event_loop->BreakLoop();
    if (m_thread->joinable())
    {
        m_thread->join();
    }
}

std::shared_ptr<Event> evThread::RegistEvent(int fd, short events, const OnEventCallback& onevent_cb)
{
    return m_event_loop->CreateEvent(fd, events, onevent_cb);
}

std::shared_ptr<Event> evThread::RegistEventSafe(int fd, short events, const OnEventCallback& onevent_cb)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return RegistEvent(fd, events, onevent_cb);
}

void evThread::Work()
{
    
    m_event_loop->StartLoop(EventLoopOpt::LOOP_NO_EXIT_ON_EMPTY);
}
}