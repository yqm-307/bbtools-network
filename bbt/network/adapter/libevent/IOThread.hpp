/**
 * @file IOThread.hpp
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <map>

#include <bbt/base/timer/Clock.hpp>
#include <bbt/base/buffer/Buffer.hpp>
#include <bbt/base/poolutil/IDPool.hpp>

#include "bbt/network/adapter/base/IOThread.hpp"
#include "bbt/network/adapter/libevent/EventLoop.hpp"

namespace bbt::network::libevent
{

class IOThread:
    public base::IOThread
{
public:
    typedef std::function<void()>                                           IOWorkFunc;
    typedef std::function<void(const bbt::buffer::Buffer&, const Errcode&)> OnRecvCallback;
    typedef std::function<void(const Errcode&)>                             OnTimeOutCallback;
    typedef std::function<void(evutil_socket_t, short, void*)>              EventCallback;

    IOThread(std::shared_ptr<EventLoop> eventloop);
    explicit IOThread(std::shared_ptr<EventLoop> eventloop, const HookCallback& thread_begin, const HookCallback& thread_end);
    virtual ~IOThread();

    virtual Errcode         Stop() override;
    std::shared_ptr<Event>  RegisterEvent(evutil_socket_t fd, short events, const OnEventCallback& onevent_cb);
    std::shared_ptr<EventLoop> 
                            GetEventLoop() const;

private:
    virtual void            WorkHandle() override;
    void                    Init();
    void                    Destory();
    void                    evWorkFunc();

protected:
    std::shared_ptr<EventLoop>  m_eventloop{nullptr};
    std::mutex                  m_mutex;
};


} // bbt::network::libevent