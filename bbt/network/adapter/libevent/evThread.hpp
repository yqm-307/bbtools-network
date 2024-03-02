#pragma once
#include "bbt/network/adapter/libevent/EventLoop.hpp"

namespace bbt::network::libevent
{

class evThread
{
public:
    evThread()
        :m_event_loop(std::make_shared<EventLoop>())
    {}

    ~evThread() { Stop(); delete m_thread; }

    void Start();
    void Stop();

    std::shared_ptr<Event> RegistEvent(int fd, short events, const OnEventCallback& onevent_cb);
    std::shared_ptr<Event> RegistEventSafe(int fd, short events, const OnEventCallback& onevent_cb);
protected:
    void Work();
private:
    std::shared_ptr<EventLoop>  m_event_loop{nullptr};
    std::thread*                m_thread{nullptr};
    std::mutex                  m_mutex;
};

}