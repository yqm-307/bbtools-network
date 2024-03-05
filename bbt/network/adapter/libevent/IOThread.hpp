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

    IOThread();
    explicit IOThread(const HookCallback& thread_begin, const HookCallback& thread_end);
    virtual ~IOThread();

    /* 阻塞的停止thread */
    virtual Errcode Stop() override;

    /**
     * @brief 在当前线程中注册一个事件
     * 
     * @param event_ptr 
     * @return EventId 如果成功返回事件id，失败返回-1
     */
    // EventId RegisterEvent(std::shared_ptr<evEvent> event_ptr);
    std::shared_ptr<Event>  RegisterEvent(evutil_socket_t fd, short events, const OnEventCallback& onevent_cb);

    const std::unique_ptr<EventLoop>& GetEventLoop() const;

private:
    virtual void            WorkHandle() override;
    /* 初始化线程内部资源 */
    void                    Init();
    /* 回收线程内部资源 */
    void                    Destory();
    void                    evWorkFunc();

protected:
    std::unique_ptr<EventLoop>  m_eventloop{nullptr};
    std::mutex                  m_mutex;
};


} // bbt::network::libevent