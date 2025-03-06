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

#include <bbt/core/clock/Clock.hpp>
#include <bbt/core/buffer/Buffer.hpp>
#include <bbt/core/crypto/BKDR.hpp>

#include <bbt/pollevent/EventLoop.hpp>

#include "bbt/network/adapter/libevent/Connection.hpp"
#include "bbt/network/adapter/base/IOThread.hpp"

namespace bbt::network::libevent
{
typedef std::function<void(ErrOpt, libevent::ConnectionSPtr /* new_conn */)>            OnAcceptCallback;
typedef std::function<void(ErrOpt)>                                                     OnErrorCallback;
typedef std::function<std::shared_ptr<libevent::IOThread>()>                            OnRegistThreadCallback;

class IOThread:
    public base::IOThread
{
    typedef bbt::pollevent::EventLoop EventLoop;
    typedef bbt::pollevent::Event Event;
public:
    typedef std::function<void()>                                           IOWorkFunc;
    typedef std::function<void(const bbt::core::Buffer&, ErrOpt)>   OnRecvCallback;
    typedef std::function<void(ErrOpt)>                       OnTimeOutCallback;
    typedef std::function<void(evutil_socket_t, short, void*)>              EventCallback;

    IOThread(std::shared_ptr<EventLoop> eventloop);
    virtual ~IOThread();

    /////////////////////// 网络操作
    /* 新连接默认在本线程运行 */
    ErrOpt    Listen(const char* ip, short port, const OnAcceptCallback& onaccept_cb, std::shared_ptr<libevent::IOThread> thread = nullptr);
    ErrOpt    UnListen(const char* ip, short port);
    ErrOpt    AsyncConnect(const char* ip, short port, int timeout_ms, const interface::OnConnectCallback& onconnect);
    ///////////////////////


    virtual ErrOpt Stop() override;
    std::shared_ptr<Event>  RegisterEvent(evutil_socket_t fd, short events, const bbt::pollevent::OnEventCallback& onevent_cb);
    std::shared_ptr<EventLoop> 
                            GetEventLoop() const;
    void                    SetOnStart(const HookCallback& on_thread_start_callback);
    void                    SetOnStop(const HookCallback& on_thread_stop_callback);
    void                    SetOnError(const OnErrorCallback& onerror);
private:
    virtual void            WorkHandle() override;
    void                    Init();
    void                    Destory();
    void                    evWorkFunc();

    void                    OnAccept(int fd, short events, const OnAcceptCallback& onaccept, std::shared_ptr<libevent::IOThread> thread);
    void                    OnConnect(
                                int sockfd,
                                EventId event,
                                short events,
                                core::clock::Timestamp<core::clock::ms> timeout,
                                const IPAddress& addr,
                                interface::OnConnectCallback onconnect);
    ErrOpt                  Connect(evutil_socket_t fd, const IPAddress& addr);

    ErrTuple<libevent::ConnectionSPtr>
                            Accept(int listenfd, std::shared_ptr<libevent::IOThread> thread);
    void                    OnError(ErrOpt error);
protected:
    struct AddressHash { std::size_t operator()(const IPAddress& addr) const { return core::crypto::BKDR::BKDRHash(addr.GetIPPort());}; };
    struct ConnectEventMapImpl
    {
        ErrOpt            AddConnectEvent(std::shared_ptr<Event> event);
        ErrOpt            DelConnectEvent(EventId event);
        std::mutex                      m_connect_mutex;
        std::map<EventId, std::shared_ptr<Event>>
                                        m_connect_events;           // 连接事件
    };

    ConnectEventMapImpl         m_impl_connect_event_map;   // connect 事件集
    std::unordered_map<IPAddress, std::shared_ptr<Event>, AddressHash>
                                m_addr2event_map;
    std::shared_ptr<EventLoop>  m_eventloop{nullptr};
    std::mutex                  m_addr2event_mutex;
    OnErrorCallback             m_on_error{nullptr};

};


} // bbt::network::libevent