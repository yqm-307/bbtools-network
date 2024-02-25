/**
 * @file EventLoop.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-23
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <bbt/network/adapter/libevent/EventLoop.hpp>
#include <bbt/network/adapter/libevent/EventBase.hpp>

namespace bbt::network::libevent
{


EventLoop::EventLoop()
    :m_io_context(new EventBase)
{
}

EventLoop::~EventLoop()
{
    delete m_io_context;
    m_io_context = nullptr;
}

Errcode EventLoop::StartLoop(EventLoopOpt opt)
{
    int err = event_base_loop(m_io_context->m_io_context, opt);

    if (err == 1) {
        return Errcode("", ErrType::ERRTYPE_EVENTLOOP_LOOP_EXIT);
    } else if (err == -1) {
        return Errcode("", ErrType::ERRTPYE_EVENTLOOP_LOOP_ERR_EXIT);
    }

    return FASTERR_NOTHING;
}

Errcode EventLoop::BreakLoop()
{
    int err = event_base_loopbreak(m_io_context->m_io_context);

    if (err < 0) {
        return Errcode("", ErrType::ERRTYPE_ERROR);
    }

    return FASTERR_NOTHING;
}

std::shared_ptr<Event> EventLoop::CreateEvent(evutil_socket_t fd, EventOpt events, const OnEventCallback& onevent_cb)
{
    auto event_sptr = std::make_shared<Event>(m_io_context, fd, events, onevent_cb);
    return event_sptr;
}


}