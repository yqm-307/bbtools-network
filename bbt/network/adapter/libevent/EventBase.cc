/**
 * @file EventBase.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-25
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <bbt/network/adapter/libevent/EventBase.hpp>

namespace bbt::network::libevent
{

EventBase::EventBase()
    :m_io_context(event_base_new())
{
    assert(m_io_context != nullptr);
}

EventBase::~EventBase()
{
    event_base_free(m_io_context);
}

int EventBase::GetEventNum()
{
    return event_base_get_num_events(m_io_context, EVENT_BASE_COUNT_ADDED);
}

event_base* EventBase::GetRawBase()
{
    return m_io_context;
}


}