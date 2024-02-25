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
#include <bbt/network/adapter/libevent/Event.hpp>
namespace bbt::network::libevent
{

void COnEventWapper(evutil_socket_t fd, short events, void* arg) 
{
    auto param = reinterpret_cast<Event::COnEventWapperParam*>(arg);
    assert(param != nullptr);
    param->m_cpp_handler(fd, events);
}

Event::Event(EventBase* base, evutil_socket_t fd, short listen_events, const OnEventCallback& onevent_cb)
{
    m_c_func_wapper_param.m_cpp_handler = onevent_cb;
    m_raw_event = event_new(base->m_io_context, fd, listen_events, COnEventWapper, &m_c_func_wapper_param);
}

Event::~Event()
{
    //XXX 反注册事件
    event_free(m_raw_event);
    m_raw_event = nullptr;
}

Errcode Event::StartListen(uint32_t timeout)
{
    timeval     tm;
    int         err;
    m_timeout = timeout;

    evutil_timerclear(&tm);
    if (m_timeout > 0) {
        tm.tv_sec  = timeout / 1000;
        tm.tv_usec = (timeout % 1000) * 1000;
    }

    err = event_add(m_raw_event, &tm);
    if (err != 0) {
        return Errcode{"event_add() failed!"};
    }

    return FASTERR_NOTHING;
}

Errcode Event::CancelListen()
{
    int         err;

    err = event_del(m_raw_event);

    if (err != 0) {
        return Errcode{"event_del() failed!"};
    }

    return FASTERR_NOTHING;
}

}