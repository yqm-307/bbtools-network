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

namespace bbt::network::libevent
{

/**
 * @brief 事件循环的选项
 * 
 */
enum EventLoopOpt
{
    LOOP_ONCE               = EVLOOP_ONCE,      /* 触发一次 */
    LOOP_NONBLOCK           = EVLOOP_NONBLOCK,  /* ??? */
    LOOP_NO_EXIT_ON_EMPTY   = EVLOOP_NO_EXIT_ON_EMPTY, /* 即使没有任何监听事件也不退出循环 */
};

/**
 * @brief 监听事件的事件类型
 * 
 */
enum EventOpt : short
{
    TIMEOUT                 = EV_TIMEOUT,       // 事件超时时触发
    READABLE                = EV_READ,          // 套接字可读时触发
    WRITEABLE               = EV_WRITE,         // 套接字可写时触发
    FD_CLOSE                = EV_CLOSED,        // 套接字关闭时触发
    PERSIST                 = EV_PERSIST,       // 设置事件监听事件为持续监听的，否则触发一次事件就结束
};

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

    Errcode StartLoop(EventLoopOpt opt);
    Errcode BreakLoop();

    std::shared_ptr<Event> CreateEvent(evutil_socket_t fd, EventOpt events, const OnEventCallback& onevent_cb);


private:
    std::shared_ptr<EventBase>  m_io_context{nullptr};
};

} // namespace bbt::network::libevent