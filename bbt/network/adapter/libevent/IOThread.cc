/**
 * @file IOThread.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <bbt/base/assert/Assert.hpp>
#include <bbt/base/net/SocketUtil.hpp>
#include <bbt/pollevent/EventLoop.hpp>
#include <bbt/pollevent/Event.hpp>
#include "bbt/network/adapter/libevent/IOThread.hpp"

namespace bbt::network::libevent
{
typedef bbt::pollevent::EventOpt EventOpt;

IOThread::IOThread(std::shared_ptr<EventLoop> eventloop)
    :m_eventloop(eventloop)
{
    Init();
}

IOThread::~IOThread()
{
    /* 等待线程停止 */
    if(m_status == IOThreadRunStatus::Running)
        Stop();
    /* 资源回收 */
    Destory();
}

void IOThread::Init()
{
    SetOnThreadBegin_Hook([](IOThreadID tid){});
    SetOnThreadEnd_Hook([](IOThreadID tid){});
}

void IOThread::Destory()
{
    m_eventloop = nullptr;

    std::lock_guard<std::mutex> _(m_addr2event_mutex);
    for (auto&& event : m_addr2event_map) {
        auto addr = event.first;
        UnListen(addr.GetIP().c_str(), addr.GetPort());
    }
}


void IOThread::evWorkFunc()
{
}

void IOThread::WorkHandle()
{
    auto err = m_eventloop->StartLoop(EVLOOP_NO_EXIT_ON_EMPTY);
    Assert(err == 0);
}

std::shared_ptr<bbt::pollevent::EventLoop> IOThread::GetEventLoop() const
{
    return m_eventloop;
}

void IOThread::SetOnStart(const HookCallback& on_thread_start_callback)
{
    SetOnThreadBegin_Hook(on_thread_start_callback);
}

void IOThread::SetOnStop(const HookCallback& on_thread_stop_callback)
{
    SetOnThreadEnd_Hook(on_thread_stop_callback);
}

void IOThread::SetOnError(const OnErrorCallback& onerror)
{
    m_on_error = onerror;
}

void IOThread::OnError(const Errcode& error)
{
    Assert(m_on_error != nullptr);
    m_on_error(error);
}

Errcode IOThread::Stop()
{
    if (m_status == Finish)
        return FASTERR_NOTHING;

    m_addr2event_map.clear();

    auto err = m_eventloop->BreakLoop();
    if (err != 0)
        return Errcode{"stop failed!"};
    
    /* 阻塞式的等待 */
    SyncWaitThreadExit();

    return FASTERR_NOTHING;
}

std::shared_ptr<bbt::pollevent::Event> IOThread::RegisterEvent(evutil_socket_t fd, short events, const bbt::pollevent::OnEventCallback& onevent_cb)
{
    auto event_sptr = m_eventloop->CreateEvent(fd, events, onevent_cb);
    return event_sptr;
}

Errcode IOThread::ConnectEventMapImpl::AddConnectEvent(std::shared_ptr<Event> event)
{
    std::lock_guard<std::mutex> lock(m_connect_mutex);
    auto [it, succ] = m_connect_events.insert(std::make_pair(event->GetEventId(), event));
    if (!succ)
        return FASTERR_ERROR("event repeat!");
    
    return FASTERR_NOTHING;
}

Errcode IOThread::ConnectEventMapImpl::DelConnectEvent(EventId eventid)
{
    std::lock_guard<std::mutex> lock(m_connect_mutex);
    auto count = m_connect_events.erase(eventid);
    if (count <= 0)
        return FASTERR_ERROR("event not found!");
    
    return FASTERR_NOTHING;
}

Errcode IOThread::Listen(const char* ip, short port, const OnAcceptCallback& onaccept_cb, std::shared_ptr<libevent::IOThread> thread)
{
    auto listen_addr = bbt::net::IPAddress(ip, port);

    std::lock_guard<std::mutex> _(m_addr2event_mutex);

    auto listen_event_it = m_addr2event_map.find(listen_addr);
    if (listen_event_it != m_addr2event_map.end())
        return Errcode{"can`t repeat regist!"};

    auto fd = bbt::net::Util::CreateListen(ip, port, true);
    if (fd < 0)
        return Errcode{"create listen socket failed! errno=" + std::to_string(errno) + ", errstr=" + std::string{strerror(errno)}};

    if (onaccept_cb == nullptr)
        return Errcode{"on accept callback is null!"};

    auto weak_this  = weak_from_this();

    // 初始化事件
    auto event = RegisterEvent(fd, EventOpt::READABLE | EventOpt::PERSIST,
    [weak_this, onaccept_cb, thread](std::shared_ptr<Event> event, short events){
        if (weak_this.expired())
            return;
        auto pthis = std::dynamic_pointer_cast<libevent::IOThread>(weak_this.lock());
        pthis->OnAccept(event->GetSocket(), events, onaccept_cb, thread);
    });

    // 注册事件
    event->StartListen(0);

    auto [it, isok] = m_addr2event_map.insert(std::make_pair(listen_addr, event));
    if (!isok)
        return Errcode{"IOThread::m_addr2event_map insert failed! ip:{" + listen_addr.GetIPPort() + '}'};
    
    return FASTERR_NOTHING;
}

void IOThread::OnAccept(int fd, short events, const OnAcceptCallback& onaccept, std::shared_ptr<libevent::IOThread> thread)
{
    if ( (events & EventOpt::READABLE) > 0 ) {
        while (true) {
            auto [err, new_conn_sptr] = Accept(fd, thread);
            if (new_conn_sptr == nullptr)
                break;
            /* 排除掉 errno = try again 的 */
            if ( (err.IsErr()) || (!err.IsErr() && new_conn_sptr != nullptr) ) {
                onaccept(err, new_conn_sptr);
                new_conn_sptr->RunInEventLoop();
            }
        }
    }
}

std::pair<Errcode, libevent::ConnectionSPtr> IOThread::Accept(int listenfd, std::shared_ptr<libevent::IOThread> thread)
{
    evutil_socket_t     fd;
    sockaddr_in         addr;
    socklen_t           len = sizeof(addr);
    fd = ::accept(listenfd, reinterpret_cast<sockaddr*>(&addr), &len);


    bbt::net::IPAddress endpoint;
    endpoint.set(addr);

    if(fd >= 0) {
        ConnectionSPtr new_conn_sptr;
        if (thread == nullptr)
            new_conn_sptr = Connection::Create(
                std::dynamic_pointer_cast<libevent::IOThread>(shared_from_this()), fd, endpoint);
        else
            new_conn_sptr = Connection::Create(
                thread, fd, endpoint);

        return {FASTERR_NOTHING, new_conn_sptr} ;
    }

    if( !(errno == EINTR ||  errno == EAGAIN || errno == ECONNABORTED) )
        OnError(Errcode{"accept() failed!"});

    return {FASTERR_NOTHING, nullptr};
}

Errcode IOThread::UnListen(const char* ip, short port)
{
    auto address = bbt::net::IPAddress(ip, port);

    std::lock_guard<std::mutex> _(m_addr2event_mutex);
    auto it = m_addr2event_map.find(address);
    if (it == m_addr2event_map.end())
        return Errcode{"not find ip:{" + address.GetIPPort() + '}'};

    m_addr2event_map.erase(it);
    auto err = it->second->CancelListen();
    if (err != 0)
        return Errcode{"cancel event failed!"};

    return FASTERR_NOTHING;
}

Errcode IOThread::AsyncConnect(const char* ip, short port, int timeout_ms, const interface::OnConnectCallback& onconnect)
{
    if (timeout_ms <= 0)
        return Errcode{"async connect, param timeout_ms can`t less then 0!"};

    int socket = bbt::net::Util::CreateConnect(ip, port, true);
    if (socket < 0)
        return Errcode{"create socket failed!"};

    bbt::net::IPAddress addr{ip, port};
    auto timeout_timestamp = bbt::timer::clock::nowAfter(bbt::timer::clock::ms(timeout_ms));
    auto event = RegisterEvent(socket, EventOpt::WRITEABLE | EventOpt::TIMEOUT | EventOpt::PERSIST,
    [=](std::shared_ptr<Event> event, short events){
        OnConnect(socket, event->GetEventId(), events, timeout_timestamp, addr, onconnect);
    });

    event->StartListen(CONNECT_TIMEOUT_MS);
    m_impl_connect_event_map.AddConnectEvent(event);

    return FASTERR_NOTHING;
}

void IOThread::OnConnect(
    int sockfd,
    EventId eventid,
    short events,
    bbt::timer::clock::Timestamp<bbt::timer::clock::ms> timeout,
    const bbt::net::IPAddress& addr,
    interface::OnConnectCallback onconnect)
{
    if (events & EventOpt::TIMEOUT ||  bbt::timer::clock::expired<bbt::timer::clock::ms>(timeout))
    {        
        onconnect(Errcode{"connect client timeout!", ErrType::ERRTYPE_CONNECT_TIMEOUT}, nullptr);
        ::close(sockfd);
        m_impl_connect_event_map.DelConnectEvent(eventid);
        return;
    }
        
    if (events & EventOpt::WRITEABLE) {
        auto err = Connect(sockfd, addr);
        if (err.IsErr() && err.Type() == ErrType::ERRTYPE_CONNECT_TRY_AGAIN) {
            return;
        }

        if (!err.IsErr()) {
            auto conn_sptr = libevent::Connection::Create(
                std::dynamic_pointer_cast<libevent::IOThread>(shared_from_this()), sockfd, addr);
            onconnect(FASTERR_NOTHING, conn_sptr);
            conn_sptr->RunInEventLoop();
            m_impl_connect_event_map.DelConnectEvent(eventid);
        }
    }
}

Errcode IOThread::Connect(evutil_socket_t fd, const bbt::net::IPAddress& addr)
{
    if (0 > ::connect(fd, addr.getsockaddr(), addr.getsocklen())) {
        int err = evutil_socket_geterror(fd);
        if (err == EINTR || err == EINPROGRESS) {
            return Errcode{"", ErrType::ERRTYPE_CONNECT_TRY_AGAIN};
        }

        if (err == ECONNREFUSED)
            return Errcode{"", ErrType::ERRTYPE_CONNECT_CONNREFUSED};
    } else {
        return Errcode{"", ErrType::ERRTYPE_NOTHING};
    }

    ::close(fd);
    return Errcode{"connect failed! undef error!"};
}

}// namespace end