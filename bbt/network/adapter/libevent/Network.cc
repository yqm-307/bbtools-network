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

Network::Network(EventBase* io_context, uint32_t sub_thread_num, const char* ip, short port)
    :m_listen_addr(ip, port),
    m_io_context(io_context),
    m_sub_loop_nums(sub_thread_num),
    m_count_down_latch(new bbt::thread::lock::CountDownLatch(sub_thread_num + 1))
{
    AssertWithInfo(m_io_context != nullptr, "io_context can`t NULL!");

    /* 初始化主线程 */
    m_main_thread = std::make_shared<libevent::IOThread>();
    /* 初始化子线程 */
    for (int i = 0; i < m_sub_loop_nums; ++i) {
        auto io_thread = std::make_shared<libevent::IOThread>();
        m_sub_threads.push_back(io_thread);
    }

    m_listen_fd = bbt::net::Util::CreateListen(ip, port, true);
    AssertWithInfo(m_listen_fd >= 0, "create listen socket failed!");
}

Network::~Network()
{
    delete m_count_down_latch;
    m_count_down_latch = nullptr;

    if (Status() == NetworkStatus::RUNNING)
        Stop();

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

    m_status = NetworkStatus::RUNNING;
}

void Network::Stop()
{
    if (m_status != NetworkStatus::RUNNING)
        return;

    for (int i = 0; i < m_sub_loop_nums; ++i) {
        if (!m_sub_threads[i]->IsRunning())
            continue;
        
        m_sub_threads[i]->Stop();
    }

    m_status = NetworkStatus::STOP;
}

Errcode Network::StartListen(const OnAcceptCallback& onaccept_cb)
{
    if (m_onaccept_event != nullptr)
        return Errcode{"repeat regist event!"};

    m_onaccept_event = m_main_thread->RegisterEventSafe(m_listen_fd, EventOpt::READABLE | EventOpt::PERSIST, 
    [this, onaccept_cb](evutil_socket_t fd, short events){
        OnAccept(fd, events, onaccept_cb);
    });

    m_onaccept_event->StartListen(50);
    
    return FASTERR_NOTHING;
}

void Network::OnError(const Errcode& err)
{
    if (m_error_handle != nullptr)
        m_error_handle(err);
}

void Network::SetOnErrorHandle(const OnErrorCallback& onerror_cb)
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

libevent::ConnectionSPtr Network::DoAccept(int listenfd)
{
    evutil_socket_t         newfd = -1;
    struct sockaddr_in      addr;
    socklen_t               len = sizeof(addr);
    bbt::net::IPAddress     endpoint;

    newfd = ::accept(listenfd, reinterpret_cast<sockaddr*>(&addr), &len);

    if ( (newfd < 0) && !(errno == EINTR ||  errno == EAGAIN || errno == ECONNABORTED) ) {
        OnError(FASTERR_ERROR("::accept() failed!"));
        return nullptr;
    }

    endpoint.set(addr);

    auto conn = Create<libevent::Connection>(
        GetAThread(),
        newfd,
        endpoint
    );

    return conn;
}

void Network::OnAccept(evutil_socket_t fd, short events, OnAcceptCallback onaccept)
{
    if ( (events & EventOpt::READABLE) > 0 ) {
        auto conn_sptr = DoAccept(fd);
        onaccept(conn_sptr);
    }
}

Errcode Network::AsyncConnect(const char* ip, short port, const interface::OnConnectCallback& onconnect)
{
    int socket = bbt::net::Util::CreateConnect(ip, port, true);    
    if (socket < 0)
        return Errcode{"create socket failed!"};

    bbt::net::IPAddress addr{ip, port};

    auto err = DoConnect(socket, addr);
    if (!err && err.Type() == ErrType::ERRTYPE_CONNECT_TRY_AGAIN) {
        auto event = m_main_thread->RegisterEventSafe(socket, EventOpt::WRITEABLE | EventOpt::TIMEOUT, 
        [this, onconnect, addr](evutil_socket_t fd, short events){
            OnConnect(fd, events, addr, onconnect);
        });

        event->StartListen(CONNECT_TIMEOUT_MS);
    }

    if (err) {
        auto conn_sptr = Create<libevent::Connection>(GetAThread(), socket, addr);
        onconnect(FASTERR_NOTHING, conn_sptr);
    }

    return FASTERR_NOTHING;
}

void Network::OnConnect(evutil_socket_t fd, short events, const bbt::net::IPAddress& addr, interface::OnConnectCallback onconnect)
{
    if (events & EventOpt::TIMEOUT) {
        onconnect(Errcode{"connect client timeout!", ErrType::ERRTYPE_CONNECT_TIMEOUT}, nullptr);
        ::close(fd);
        return;
    }
        
    if (events & EventOpt::WRITEABLE) {
        auto err = DoConnect(fd, addr);
        if (!err && err.Type() == ErrType::ERRTYPE_CONNECT_TRY_AGAIN) {
            auto event = m_main_thread->RegisterEventSafe(fd, EventOpt::WRITEABLE | EventOpt::TIMEOUT, 
            [this, onconnect, addr](evutil_socket_t fd, short events){
                OnConnect(fd, events, addr, onconnect);
            });

            event->StartListen(CONNECT_TIMEOUT_MS);
        }

        if (err) {
            auto conn_sptr = Create<libevent::Connection>(GetAThread(), fd, addr);
            onconnect(FASTERR_NOTHING, conn_sptr);
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



} // namespace bbt::network::libevent