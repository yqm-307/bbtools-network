/**
 * @file Event.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-25
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <bbt/base/uuid/EasyID.hpp>
#include <bbt/network/adapter/libevent/Event.hpp>
namespace bbt::network::libevent
{

EventId Event::GenerateID()
{
    return bbt::uuid::EasyID<bbt::uuid::emEasyID::EM_AUTO_INCREMENT_SAFE, 2>::GenerateID();
}

void COnEventWapper(evutil_socket_t fd, short events, void* arg) 
{
    auto pthis = reinterpret_cast<Event*>(arg);
    assert(pthis != nullptr);
    pthis->m_c_func_wapper_param.m_cpp_handler(pthis->shared_from_this(), events);
}

Event::Event(EventBase* base, evutil_socket_t fd, short listen_events, const OnEventCallback& onevent_cb)
    :m_id(GenerateID())
{
    Assert(base != nullptr);
    m_c_func_wapper_param.m_cpp_handler = onevent_cb;
    m_raw_event = event_new(base->m_io_context, fd, listen_events, COnEventWapper, this);
}

Event::~Event()
{
    auto err = CancelListen();
    DebugAssertWithInfo(!err.IsErr(), "it`s a wrong!");
    event_free(m_raw_event);

    m_raw_event = nullptr;
}

Errcode Event::StartListen(uint32_t timeout)
{
    timeval     tm;
    timeval*    tmptr = nullptr;
    int         err;
    m_timeout = timeout;

    if (m_timeout > 0) {
        evutil_timerclear(&tm);
        tm.tv_sec  = timeout / 1000;
        tm.tv_usec = (timeout % 1000) * 1000;
        tmptr = &tm;
    }

    err = event_add(m_raw_event, tmptr);
    if (err != 0) {
        return Errcode{"event_add() failed!"};
    }

    return FASTERR_NOTHING;
}

Errcode Event::CancelListen()
{
    int         err;

    err = event_del(m_raw_event);
    evutil_socket_t socket = GetSocket();

    if (socket >= 0)
        ::close(socket);

    if (err != 0) {
        return Errcode{"event_del() failed!"};
    }

    return FASTERR_NOTHING;
}

EventId Event::GetEventId()
{
    return m_id;
}

int Event::GetSocket()
{
    return event_get_fd(m_raw_event);
}

short Event::GetEvents()
{
    return event_get_events(m_raw_event);
}

} // namespace