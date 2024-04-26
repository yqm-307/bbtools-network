/**
 * @file EventBase.hpp
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <event2/event.h>
#include <cassert>

#include <bbt/network/Errcode.hpp>

namespace bbt::database::redis { class AsyncConnection; }

namespace bbt::network::libevent
{

class EventBase
{
    friend class Event;
    friend class EventLoop;
    friend class bbt::database::redis::AsyncConnection;
public:
    EventBase();
    ~EventBase();

    int GetEventNum();
private:
    event_base* m_io_context{nullptr};
};

} // namespace bbt::network::libevent