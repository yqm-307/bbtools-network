/**
 * @file EventLoop.hpp
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-23
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <bbt/network/Define.hpp>
#include <bbt/network/adapter/libevent/Event.hpp>
#include <bbt/base/Attribute.hpp>

namespace bbt::network::libevent
{

/**
 * @brief 事件循环的选项
 * 
 */
enum EventLoopOpt
{
    LOOP_NORMAL             = 0,                /* 默认循环，没有事件退出 */
    LOOP_ONCE               = EVLOOP_ONCE,      /* 触发一次 */
    LOOP_NONBLOCK           = EVLOOP_NONBLOCK,  /* ??? */
    LOOP_NO_EXIT_ON_EMPTY   = EVLOOP_NO_EXIT_ON_EMPTY, /* 即使没有任何监听事件也不退出循环 */
};

class EventLoop
{
public:
    /* base 会在析构时释放内存 */
    explicit EventLoop(EventBase* base);
    EventLoop();
    ~EventLoop();

    BBTATTR_FUNC_RetVal Errcode StartLoop(int opt);
    BBTATTR_FUNC_RetVal Errcode BreakLoop();

    int GetEventNum();

    std::shared_ptr<Event> CreateEvent(evutil_socket_t fd, short events, const OnEventCallback& onevent_cb);


private:
    EventBase*  m_io_context{nullptr};
};

} // namespace bbt::network::libevent