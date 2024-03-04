/**
 * @file Network.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <bbt/base/net/SocketUtil.hpp>
#include <bbt/network/adapter/libevent/Network.hpp>
#include <event2/util.h>

namespace bbt::network::libevent
{

void WaitForCountDown(bbt::thread::lock::CountDownLatch* latch)
{
    latch->down();
    latch->wait();
}

Network::Network(uint32_t sub_thread_num, const char* ip, short port)
    :m_listen_addr(ip, port),
    m_sub_loop_nums(sub_thread_num),
    m_count_down_latch(new bbt::thread::lock::CountDownLatch(sub_thread_num + 1))
{
    /* 初始化主线程 */
    m_main_thread = std::make_shared<libevent::IOThread>([this](IOThreadID){ WaitForCountDown(m_count_down_latch); }, nullptr);
    /* 初始化子线程 */
    for (int i = 0; i < m_sub_loop_nums; ++i) {
        auto io_thread = std::make_shared<libevent::IOThread>([this](IOThreadID){ WaitForCountDown(m_count_down_latch); }, nullptr);
        m_sub_threads.push_back(io_thread);
    }

    m_listen_fd = bbt::net::Util::CreateListen(ip, port, true);
    AssertWithInfo(m_listen_fd >= 0, "create listen socket failed!");
}

Network::~Network()
{
    delete m_count_down_latch;
    m_count_down_latch = nullptr;

    if (Status() == NetworkStatus::RUNNING) {
        Stop();
    }

}


void Network::Start()
{
    if (m_status != NetworkStatus::DEFAULT)
        return;

    m_status = NetworkStatus::STARTING;

    // 启动子线程
    for (int i = 0; i < m_sub_loop_nums; ++i)
        m_sub_threads[i]->Start();

    // 启动主线程
    m_main_thread->Start();

    m_count_down_latch->wait();
    m_status = NetworkStatus::RUNNING;
}

void Network::Stop()
{
    if (m_status != NetworkStatus::RUNNING)
        return;

    /*停止接收线程*/
    StopMainThread();

    /* 停止IO线程 */
    StopSubThread();

    m_status = NetworkStatus::STOP;
}

void Network::StopMainThread()
{
    if (m_main_thread->IsRunning()) {
        if (m_onaccept_event != nullptr)
            m_onaccept_event->CancelListen();
        Assert(m_main_thread->Stop());
    }
}

void Network::StopSubThread()
{
    for (int i = 0; i < m_sub_loop_nums; ++i) {
        if (!m_sub_threads[i]->IsRunning())
            continue;
        
        Assert(m_sub_threads[i]->Stop());
    }

    m_status = NetworkStatus::STOP;
}

Errcode Network::StartListen(const OnAcceptCallback& onaccept_cb)
{
    if (m_onaccept_event != nullptr)
        return Errcode{"repeat regist event!"};

    m_onaccept_event = m_main_thread->RegisterEventSafe(m_listen_fd, EventOpt::READABLE | EventOpt::PERSIST, 
    [this, onaccept_cb](std::shared_ptr<Event> event, short events){
        OnAccept(event->GetSocket(), events, onaccept_cb);
    });

    m_onaccept_event->StartListen(50);
    
    return FASTERR_NOTHING;
}

void Network::OnError(const Errcode& err)
{
    if (m_error_handle != nullptr)
        m_error_handle(err);
}

void Network::SetOnErrorHandle(const OnNetworkErrorCallback& onerror_cb)
{
    m_error_handle = onerror_cb;
}

NetworkStatus Network::Status()
{
    return m_status;
}

Network::ThreadSPtr Network::GetAThread()
{
    int index = m_cur_max_conn_count % m_sub_loop_nums;
    return m_sub_threads[index];
}

std::pair<Errcode, libevent::ConnectionSPtr> Network::DoAccept(int listenfd)
{
    evutil_socket_t fd;
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    fd = ::accept(m_listen_fd, reinterpret_cast<sockaddr*>(&addr), &len);

    bbt::net::IPAddress endpoint;
    endpoint.set(addr);

    if(fd >= 0) {
        auto new_conn_sptr = Create<libevent::Connection>(GetAThread(), fd, endpoint);
        return {FASTERR_NOTHING, new_conn_sptr} ;
    }

    if( !(errno == EINTR ||  errno == EAGAIN || errno == ECONNABORTED) )
        OnError(Errcode{"accept() failed!"});

    return {FASTERR_NOTHING, nullptr};
}

void Network::OnAccept(evutil_socket_t fd, short events, OnAcceptCallback onaccept)
{
    if ( (events & EventOpt::READABLE) > 0 ) {
        while (true) {
            auto [err, new_conn_sptr] = DoAccept(fd);
            if (new_conn_sptr == nullptr)
                break;
            /* 排除掉 errno = try again 的 */
            if ( (!err) || (err && new_conn_sptr != nullptr) ) {
                onaccept(err, new_conn_sptr);
                new_conn_sptr->RunInEventLoop();
            }
        }
    }
}

Errcode Network::AsyncConnect(const char* ip, short port, const interface::OnConnectCallback& onconnect)
{
    int socket = bbt::net::Util::CreateConnect(ip, port, true);
    if (socket < 0)
        return Errcode{"create socket failed!"};

    bbt::net::IPAddress addr{ip, port};

    auto event = m_main_thread->RegisterEventSafe(socket, EventOpt::WRITEABLE | EventOpt::TIMEOUT,
    [this, onconnect, addr](std::shared_ptr<Event> event, short events){
        OnConnect(event, events, addr, onconnect);
        m_impl_connect_event_map.DelConnectEvent(event);
    });

    event->StartListen(10);
    m_impl_connect_event_map.AddConnectEvent(event);

    return FASTERR_NOTHING;
}

void Network::OnConnect(std::shared_ptr<Event> event, short events, const bbt::net::IPAddress& addr, interface::OnConnectCallback onconnect)
{
    int sockfd = event->GetSocket();

    if (events & EventOpt::TIMEOUT) {
        onconnect(Errcode{"connect client timeout!", ErrType::ERRTYPE_CONNECT_TIMEOUT}, nullptr);
        ::close(sockfd);
        return;
    }
        
    if (events & EventOpt::WRITEABLE) {
        auto err = DoConnect(sockfd, addr);
        if (!err && err.Type() == ErrType::ERRTYPE_CONNECT_TRY_AGAIN) {
            auto event = m_main_thread->RegisterEventSafe(sockfd, EventOpt::WRITEABLE | EventOpt::TIMEOUT, 
            [this, onconnect, addr](std::shared_ptr<Event> event, short events){
                OnConnect(event, events, addr, onconnect);
                m_impl_connect_event_map.DelConnectEvent(event);
            });

            event->StartListen(CONNECT_TIMEOUT_MS);
            m_impl_connect_event_map.AddConnectEvent(event);
        }

        if (err) {
            auto conn_sptr = Create<libevent::Connection>(GetAThread(), sockfd, addr);
            onconnect(FASTERR_NOTHING, conn_sptr);
            conn_sptr->RunInEventLoop();
        }
    }
}

Errcode Network::DoConnect(evutil_socket_t fd, const bbt::net::IPAddress& addr)
{
    if (0 > ::connect(fd, addr.getsockaddr(), addr.getsocklen())) {
        int err = evutil_socket_geterror(fd);
        if (err == EINTR || err == EINPROGRESS)
            return Errcode{"", ErrType::ERRTYPE_CONNECT_TRY_AGAIN};

        if (err == ECONNREFUSED)
            return Errcode{"", ErrType::ERRTYPE_CONNECT_CONNREFUSED};
    } else {
        return Errcode{"", ErrType::ERRTYPE_NOTHING};
    }

    ::close(fd);
    return Errcode{"connect failed! undef error!"};
}

Errcode Network::ConnectEventMapImpl::AddConnectEvent(std::shared_ptr<Event> event)
{
    std::lock_guard<std::mutex> lock(m_connect_mutex);
    auto [it, succ] = m_connect_events.insert(std::make_pair(event->GetEventId(), event));
    if (!succ)
        return FASTERR_ERROR("event repeat!");
    
    return FASTERR_NOTHING;
}

Errcode Network::ConnectEventMapImpl::DelConnectEvent(std::shared_ptr<Event> event)
{
    std::lock_guard<std::mutex> lock(m_connect_mutex);
    auto count = m_connect_events.erase(event->GetEventId());
    if (count <= 0)
        return FASTERR_ERROR("event not found!");
    
    return FASTERR_NOTHING;
}

} // namespace bbt::network::libevent