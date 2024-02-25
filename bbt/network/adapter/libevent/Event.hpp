/**
 * @file Event.hpp
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <bbt/network/Define.hpp>

#include "EventBase.hpp"

namespace bbt::network::libevent
{

/**
 * @brief 封装libevent的事件触发函数，当有关注的事件发生时通过此函数
 * 回调通知到关注者
 */
typedef std::function<void(evutil_socket_t /*fd*/, short /*events flag*/)> OnEventCallback;

class Event
{
public:
    typedef struct {OnEventCallback m_cpp_handler{nullptr};} COnEventWapperParam;

    Event(EventBase* base, evutil_socket_t fd, short listen_events, const OnEventCallback& onevent_cb);
    ~Event();

    /**
     * @brief 开始监听事件
     * 
     * @param timeout 超时时间，单位ms
     * @return Errcode
     */
    Errcode StartListen(uint32_t timeout);

    /**
     * @brief 取消对此事件的监听
     * 
     * @return Errcode 
     */
    Errcode CancelListen();
private:
    event*                  m_raw_event;                    /* 包装的libevent事件句柄 */
    COnEventWapperParam*    m_c_func_wapper_param{nullptr}; /* cfunc转发到std::function包装器 */
    uint32_t                m_timeout{0};
};




//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////


void COnEventWapper(evutil_socket_t fd, short events, void* arg) 
{
    auto param = reinterpret_cast<Event::COnEventWapperParam*>(arg);
    assert(param != nullptr);
}

Event::Event(EventBase* base, evutil_socket_t fd, short listen_events, const OnEventCallback& onevent_cb)
{
    m_raw_event = event_new(base->m_io_context, fd, listen_events, COnEventWapper, m_c_func_wapper_param);
}

Event::~Event()
{
    event_free(m_raw_event);
    m_raw_event = nullptr;

    delete m_c_func_wapper_param;
    m_c_func_wapper_param = nullptr;
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

    int err = event_add(m_raw_event, &tm);
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

};