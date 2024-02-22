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

namespace bbt::network::libevent
{

class EventBase
{
    friend class Event;

public:
    EventBase();
    ~EventBase();

    
private:
    event_base* m_io_context{nullptr};
};

EventBase::EventBase()
    :m_io_context(event_base_new())
{
    assert(m_io_context != nullptr);
}

EventBase::~EventBase()
{
    event_base_free(m_io_context);
}

}