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
class Event;
/**
 * @brief 监听事件的事件类型
 * 
 */
enum EventOpt : short
{
    TIMEOUT                 = EV_TIMEOUT,       // 事件超时时触发
    READABLE                = EV_READ,          // 套接字可读时触发
    WRITEABLE               = EV_WRITE,         // 套接字可写时触发
    CLOSE                   = EV_CLOSED,        // 套接字关闭时触发
    FINALIZE                = EV_FINALIZE,      // 对端关闭事件，相比Close支持更广，监听对端关闭请使用此事件
    PERSIST                 = EV_PERSIST,       // 设置事件监听事件为持续监听的，否则触发一次事件就结束
    SIGNAL                  = EV_SIGNAL,        // 系统事件
};

/**
 * @brief 封装libevent的事件触发函数，当有关注的事件发生时通过此函数
 * 回调通知到关注者
 */
typedef std::function<void(std::shared_ptr<Event> /*this*/, short /*events*/)> OnEventCallback;

class Event:
    public std::enable_shared_from_this<Event>
{
    friend void COnEventWapper(evutil_socket_t fd, short events, void* arg);
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

    EventId GetEventId();

    int     GetSocket();

    short   GetEvents();
private:
    static EventId GenerateID();
private:
    EventId                         m_id{0};
    event*                          m_raw_event;           /* 包装的libevent事件句柄 */
    COnEventWapperParam             m_c_func_wapper_param; /* cfunc转发到std::function包装器 */
    uint32_t                        m_timeout{0};
};

};