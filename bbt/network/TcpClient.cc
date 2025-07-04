#include <iostream>
#include <bbt/core/clock/Clock.hpp>
#include <bbt/core/net/SocketUtil.hpp>
#include <bbt/pollevent/Event.hpp>
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/network/TcpClient.hpp>
#include <bbt/network/detail/Connection.hpp>

using namespace bbt::core::errcode;

namespace bbt::network
{

TcpClient::TcpClient(PrivateTag, std::shared_ptr<EvThread> evthread):
    m_ev_thread(evthread),
    m_on_err([](auto connid, auto& err){ std::cerr << "[TcpServer::DefaultErr] connid=" << connid << "\terr="<< err.CWhat() << std::endl; })
{
}

std::shared_ptr<TcpClient> TcpClient::Create(std::shared_ptr<EvThread> evthread)
{
    return std::make_shared<TcpClient>(PrivateTag{}, evthread);
}

ErrOpt TcpClient::AsyncConnect(const bbt::core::net::IPAddress& addr, int timeout)
{    
    std::lock_guard<std::mutex> _(m_connect_mtx);

    int fd = -1;
    m_serv_addr = addr;
    m_connect_timeout = timeout >= 0 ? timeout : 0;
    
    if (IsConnected())
        return FASTERR_ERROR("is connected!");

    if (m_connect_event != nullptr)
        return FASTERR_ERROR("already connecting!");

    auto thread = _GetThread();
    if (thread == nullptr)
        return FASTERR_ERROR("evthread is null!");
    
    if (auto err = bbt::core::net::CreateConnect(addr.GetIP().c_str(), addr.GetPort(), true); err.IsErr())
        return err.Err();
    else
        fd = err.Ok();
    
    m_connect_event = thread->RegisterEvent(fd, EventOpt::WRITEABLE | EventOpt::TIMEOUT | EventOpt::PERSIST,
    [weak_this{weak_from_this()}](int fd, short events, EventId id)
    {
        if (auto shared_this = weak_this.lock(); shared_this != nullptr)
            shared_this->_DoConnectThreadSafe(fd, events);
    });

    if (m_connect_event->StartListen(m_connect_timeout) != 0)
    {
        m_connect_event = nullptr;
        return FASTERR_ERROR("event start listen failed!");
    }

    return FASTERR_NOTHING;
}

core::errcode::ErrOpt TcpClient::Connect(const bbt::core::net::IPAddress& addr, int timeout)
{
    std::lock_guard<std::mutex> _(m_connect_mtx);
    int fd = -1;

    if (IsConnected())
        return FASTERR_ERROR("is connected!");

    if (m_connect_event != nullptr)
        return FASTERR_ERROR("already connecting!");
    
    if (auto err = bbt::core::net::CreateConnect(addr.GetIP().c_str(), addr.GetPort(), false); err.IsErr())
        return err.Err();
    else
        fd = err.Ok();

    m_serv_addr = addr;
    m_connect_timeout = timeout >= 0 ? timeout : 0;

    _DoConnect(fd, EventOpt::WRITEABLE);

    if (m_conn == nullptr)
        return FASTERR_ERROR("connect failed!");

    return FASTERR_NOTHING;
}

core::errcode::ErrOpt TcpClient::ReConnect()
{
    return AsyncConnect(m_serv_addr, m_connect_timeout);
}


void TcpClient::_DoConnect(int socket, short events)
{
    struct sockaddr_in serv_addr;
    socklen_t addr_len = sizeof(serv_addr);

    if (events & EventOpt::TIMEOUT) {
        if (m_on_connect) m_on_connect(-1, FASTERR_ERROR("connect timeout!"));
        goto ConnectFinal;
    }

    if (events & EventOpt::WRITEABLE) {
        if (auto err = m_serv_addr.GetRawData(reinterpret_cast<sockaddr*>(&serv_addr), addr_len); err.has_value()) {
            if (m_on_err) m_on_err(-1, err.value());
            goto ConnectFinal;
        }

        if (::connect(socket, reinterpret_cast<sockaddr*>(&serv_addr), addr_len) != 0) {
            int err = evutil_socket_geterror(socket);
            if (err == EINTR || err == EINPROGRESS) {
                return;
            }

            if (err == ECONNREFUSED) {
                if (m_on_connect) m_on_connect(-1, FASTERR_ERROR("connect refused!"));
                goto ConnectFinal;
            }
            else {
                if (m_on_connect) m_on_connect(-1, FASTERR_ERROR("connect error: " + std::string(evutil_socket_error_to_string(err))));
                goto ConnectFinal;
            }
        }
    }

    m_conn = detail::Connection::Create(m_ev_thread, socket, m_serv_addr);
    if (m_on_connect) m_on_connect(m_conn->GetConnId(), FASTERR_NOTHING);
    _InitConnection(m_conn);

ConnectFinal:
    // connect 处理完毕，销毁事件和连接
    m_connect_event = nullptr;
}

void TcpClient::_DoConnectThreadSafe(int socket, short events)
{
    std::lock_guard<std::mutex> _(m_connect_mtx);
    self:_DoConnect(socket, events);
}

void TcpClient::Init()
{
    callbacks.on_close_callback =
    [weak_this{weak_from_this()}](ConnId connid, const IPAddress& addr)
    {
        if (auto shared_this = weak_this.lock(); shared_this != nullptr) {
            shared_this->_OnClose(connid);
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
                shared_this->m_on_err(conn->GetConnId(), Errcode{"no register ontimeout!", emErr::ERRTYPE_ERROR});
        }
    };
}

void TcpClient::_InitConnection(std::shared_ptr<detail::Connection> conn)
{
    Assert(conn != nullptr);
    Assert(callbacks.on_recv_callback != nullptr);
    conn->SetOpt_CloseTimeoutMS(m_connection_timeout);
    conn->SetOpt_Callbacks(callbacks);
    conn->RunInEventLoop();
}

void TcpClient::_OnClose(ConnId id)
{
    {
        std::lock_guard<std::mutex> _(m_connect_mtx);
        m_conn = nullptr;
        m_connect_event = nullptr;
    }
    if (m_on_close)
        m_on_close(id);
    else
        m_on_err(id, Errcode{"no register onclose!", emErr::ERRTYPE_ERROR});
}

std::shared_ptr<EvThread> TcpClient::_GetThread()
{
    return m_ev_thread;
}

ErrOpt TcpClient::Send(const bbt::core::Buffer& buffer)
{
    if (m_conn == nullptr)
        return FASTERR_ERROR("connection is null!");

    return m_conn->AsyncSend(buffer.Peek(), buffer.Size());
}

ErrOpt TcpClient::Close()
{
    m_conn->Close();
    m_conn = nullptr;
    return FASTERR_NOTHING;
}

bool TcpClient::IsConnected()
{
    return m_conn != nullptr && m_conn->IsConnected();
}

ConnId TcpClient::GetConnId()
{
    std::lock_guard<std::mutex> _(m_connect_mtx);
    if (m_conn == nullptr)
        return -1;

    return m_conn->GetConnId();
}




}; // namespace bbt::network