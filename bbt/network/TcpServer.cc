#include <iostream>
#include <bbt/network/TcpServer.hpp>
#include <bbt/core/net/SocketUtil.hpp>
#include <bbt/pollevent/Event.hpp>
#include <bbt/network/detail/Connection.hpp>

using namespace bbt::core::errcode;

namespace bbt::network
{

TcpServer::TcpServer(PrivateTag, std::shared_ptr<EvThread> evthread):
    m_thread_pool({evthread}),
    m_thread_count(1),
    m_on_err([](auto connid, auto& err){ std::cerr << "[TcpServer::DefaultErr] connid=" << connid << "\terr="<< err.CWhat() << std::endl; })
{
}

TcpServer::TcpServer(PrivateTag, int nthread):
    m_thread_pool(std::vector<std::shared_ptr<EvThread>>(nthread)),
    m_thread_count(nthread),
    m_on_err([](auto connid, auto& err){ std::cerr << "[TcpServer::DefaultErr] connid=" << connid << "\terr="<< err.CWhat() << std::endl; })
{
    for (int i = 0; i < nthread; ++i) {
        m_thread_pool[i] = std::make_shared<EvThread>();
    }
}

TcpServer::TcpServer(PrivateTag, const std::vector<std::shared_ptr<EvThread>>& evthreads):
    m_thread_pool(evthreads),
    m_thread_count(evthreads.size()),
    m_on_err([](auto connid, auto& err){ std::cerr << "[TcpServer::DefaultErr] connid=" << connid << "\terr="<< err.CWhat() << std::endl; })
{
}

TcpServer::~TcpServer()
{
    StopListen();
}

std::shared_ptr<TcpServer> TcpServer::Create(std::shared_ptr<EvThread> evthread)
{
    return std::make_shared<TcpServer>(PrivateTag{}, evthread);
}

std::shared_ptr<TcpServer> TcpServer::Create(int nthread)
{
    return std::make_shared<TcpServer>(PrivateTag{}, nthread);
}

std::shared_ptr<TcpServer> TcpServer::Create(const std::vector<std::shared_ptr<EvThread>>& evthreads)
{
    return std::make_shared<TcpServer>(PrivateTag{}, evthreads);
}

void TcpServer::Init()
{
    callbacks.on_close_callback =
    [weak_this{weak_from_this()}](ConnId connid, const IPAddress& addr)
    {
        if (auto shared_this = weak_this.lock(); shared_this != nullptr) {
            shared_this->OnClose(connid);
        }
    };

    callbacks.on_err_callback = [weak_this{weak_from_this()}](auto connid, auto err){
        if (auto shared_this = weak_this.lock(); shared_this != nullptr && shared_this->m_on_err)
            shared_this->m_on_err(connid, err);
    };

    callbacks.on_recv_callback =
    [weak_this{weak_from_this()}](detail::ConnectionSPtr conn, const char* data, size_t len)
    {
        if (auto shared_this = weak_this.lock(); shared_this != nullptr) {
            if (shared_this->m_on_recv)
                shared_this->m_on_recv(conn->GetConnId(), bbt::core::Buffer{data, len});
            else
                shared_this->m_on_err(conn->GetConnId(), Errcode{"no register onrecv!", emErr::ERRTYPE_ERROR});
        }
    };

    callbacks.on_send_callback =
    [weak_this{weak_from_this()}](detail::ConnectionSPtr conn, ErrOpt err, size_t send_succ_len)
    {
        if (auto shared_this = weak_this.lock(); shared_this != nullptr) {
            if (shared_this->m_on_send)
                shared_this->m_on_send(conn->GetConnId(), err, send_succ_len);
            else
                shared_this->m_on_err(conn->GetConnId(), Errcode{"no register onsend!", emErr::ERRTYPE_ERROR});
        }
    };

    callbacks.on_timeout_callback =
    [weak_this{weak_from_this()}](detail::ConnectionSPtr conn)
    {
        if (auto shared_this = weak_this.lock(); shared_this != nullptr) {
            if (shared_this->m_on_timeout)
                shared_this->m_on_timeout(conn->GetConnId());
            else
                shared_this->m_on_err(conn->GetConnId(), Errcode{"no register onsend!", emErr::ERRTYPE_ERROR});
        }
    };

    for (auto& thread : m_thread_pool) {
        if (thread != nullptr)
            thread->Start();
    }
}

bbt::core::errcode::ErrOpt TcpServer::AsyncListen(const bbt::core::net::IPAddress& listen_addr, const OnAcceptFunc& onaccept_cb)
{
    std::lock_guard<std::mutex> _(m_listen_mtx);

    if (m_listen_event != nullptr)
        return Errcode{"already listening!", ERRTYPE_ERROR};

    if (auto rlt = CreateListen(listen_addr.GetIP().c_str(), listen_addr.GetPort(), true); rlt.IsErr())
        return rlt.Err();
    else
        m_listen_fd = rlt.Ok();

    if (m_listen_fd < 0)
        return Errcode{"create listen socket failed! errno=" + std::to_string(errno) + ", errstr=" + std::string{strerror(errno)}, ERRTYPE_ERROR};

    if (onaccept_cb == nullptr)
        return Errcode{"on accept callback is null!", ERRTYPE_ERROR};

    // 初始化事件
    m_listen_event = GetThread()->RegisterEvent(m_listen_fd, EventOpt::READABLE | EventOpt::PERSIST,
    [weak_this{weak_from_this()}, onaccept_cb, thread{GetThread()}](int fd, short events, EventId evetid){
        if (auto shared_this = weak_this.lock(); shared_this != nullptr) {
            auto pthis = std::static_pointer_cast<TcpServer>(shared_this);
            pthis->_Accept(fd, events, onaccept_cb, thread);
        }
    });

    // 注册事件
    Assert(m_listen_event->StartListen(0) == 0);
    m_listen_addr = listen_addr;
    return FASTERR_NOTHING;
}

bbt::core::errcode::ErrOpt TcpServer::StopListen()
{
    std::lock_guard<std::mutex> _(m_listen_mtx);

    if (m_listen_event == nullptr)
        return Errcode{"not listening!", ERRTYPE_ERROR};
    
    auto err = m_listen_event->CancelListen();
    if (err != 0)
        return Errcode{"cancel event failed!", ERRTYPE_ERROR};

    m_listen_event = nullptr;
    if (m_listen_fd > 0)
        ::close(m_listen_fd);

    m_listen_fd = -1;
    return FASTERR_NOTHING;
}

bbt::core::net::IPAddress TcpServer::GetListenAddress()
{
    std::lock_guard<std::mutex> _(m_listen_mtx);
    return m_listen_addr;
}

bool TcpServer::IsListening()
{
    std::lock_guard<std::mutex> _(m_listen_mtx);
    return m_listen_event != nullptr;
}


void TcpServer::_Accept(int listenfd, short events, const OnAcceptFunc& onaccept, std::shared_ptr<EvThread> thread)
{
    evutil_socket_t fd = -1;
    sockaddr_in     client_addr;
    socklen_t       len = sizeof(client_addr);
    IPAddress       endpoint;
    std::shared_ptr<detail::Connection> new_conn_sptr = nullptr;

    if (events & EventOpt::READABLE == 0) {
        return;
    }

    // 有连接，一直接收
    while (true)
    {
        fd = ::accept(listenfd, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (fd < 0) break;

        endpoint.From(reinterpret_cast<sockaddr*>(&client_addr), len);
        new_conn_sptr = detail::Connection::Create(thread, fd, endpoint);
        // 保存连接
        {
            std::lock_guard<std::mutex> _(m_conn_map_mutex);
            m_conn_map[new_conn_sptr->GetConnId()] = new_conn_sptr;
        }
        onaccept(new_conn_sptr->GetConnId());
        _InitConnection(new_conn_sptr);
    }
}

void TcpServer::SetTimeout(int connection_timeout)
{
    m_connection_timeout = connection_timeout;
}

ErrOpt TcpServer::Send(ConnId connid, const bbt::core::Buffer& buffer)
{
    std::lock_guard<std::mutex> _(m_conn_map_mutex);
    auto it = m_conn_map.find(connid);
    if (it == m_conn_map.end())
        return Errcode{"connid not found!", ERRTYPE_ERROR};
    
    auto conn = it->second;
    return conn->AsyncSend(buffer.Peek(), buffer.Size());
}

void TcpServer::Close(ConnId connid)
{
    std::shared_ptr<detail::Connection> conn = nullptr;
    {
        std::lock_guard<std::mutex> _(m_conn_map_mutex);
        auto it = m_conn_map.find(connid);
        if (it == m_conn_map.end())
            return;
        
        auto conn = it->second;
    }

    if (conn)
        conn->Close();
}

detail::ConnectionSPtr TcpServer::GetConnection(ConnId connid)
{
    std::lock_guard<std::mutex> _(m_conn_map_mutex);
    auto it = m_conn_map.find(connid);
    if (it == m_conn_map.end())
        return nullptr;
    
    return it->second;
}

void TcpServer::OnTimeout(ConnId connid)
{
    if (m_on_timeout != nullptr)
        m_on_timeout(connid);
}

void TcpServer::OnSend(ConnId connid, ErrOpt err, size_t send_len)
{
    if (m_on_send != nullptr)
        m_on_send(connid, err, send_len);
}

void TcpServer::OnRecv(ConnId connid, bbt::core::Buffer& buffer)
{
    if (m_on_recv != nullptr)
        m_on_recv(connid, buffer);
}

void TcpServer::OnClose(ConnId connid)
{
    {
        std::unique_lock<std::mutex> _{m_conn_map_mutex};
        auto it = m_conn_map.find(connid);
        if (it != m_conn_map.end())
            m_conn_map.erase(it);
    }

    if (m_on_close != nullptr)
        m_on_close(connid);
    else
        m_on_err(connid, Errcode{"no register onclose!", emErr::ERRTYPE_ERROR});
}

std::shared_ptr<EvThread> TcpServer::GetThread()
{
    m_load_blance = m_load_blance + 1;
    return m_thread_pool[m_load_blance % m_thread_count];
}

void TcpServer::_InitConnection(std::shared_ptr<detail::Connection> conn)
{
    Assert(conn != nullptr);
    Assert(callbacks.on_recv_callback != nullptr);
    conn->SetOpt_CloseTimeoutMS(m_connection_timeout);
    conn->SetOpt_Callbacks(callbacks);
    conn->RunInEventLoop();
}


}