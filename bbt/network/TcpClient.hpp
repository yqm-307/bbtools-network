#pragma once
#include <bbt/network/detail/Define.hpp>

namespace bbt::network
{

class TcpClient final:
    public std::enable_shared_from_this<TcpClient>
{
public:
    TcpClient(std::shared_ptr<EvThread> evthread);
    ~TcpClient() = default;

    void            Init();

    ErrOpt          AsyncConnect(const bbt::core::net::IPAddress& addr, int timeout);
    ErrOpt          Send(const bbt::core::Buffer& buffer);
    ErrOpt          Close();
    bool            IsConnected();
    ConnId          GetConnId();

    void            SetConnectionTimeout(int timeout) { m_connection_timeout = timeout; }
    void            SetOnConnect(const OnConnectFunc& on_connect) { m_on_connect = on_connect; }
    void            SetOnTimeout(const OnTimeoutFunc& on_timeout) { m_on_timeout = on_timeout; }
    void            SetOnClose(const OnCloseFunc& on_close) { m_on_close = on_close; }
    void            SetOnSend(const OnSendFunc& on_send) { m_on_send = on_send; }
    void            SetOnRecv(const OnRecvFunc& on_recv) { m_on_recv = on_recv; }
    void            SetOnErr(const OnErrFunc& on_err) {m_on_err = on_err; }
private:
    std::shared_ptr<EvThread> _GetThread();
    void            _DoConnect(int socket, short events);
    void            _InitConnection(std::shared_ptr<detail::Connection> conn);
    void            _OnClose(ConnId id);
private:
    std::shared_ptr<EvThread> m_ev_thread{nullptr};

    detail::ConnCallbacks callbacks;

    IPAddress       m_serv_addr;
    detail::ConnectionSPtr m_conn{nullptr};
    int             m_connect_timeout{10000};
    int             m_connection_timeout{10000};
    std::shared_ptr<Event> m_connect_event{nullptr};
    std::mutex      m_connect_mtx;

    OnCloseFunc     m_on_close{nullptr};
    OnSendFunc      m_on_send{nullptr};
    OnRecvFunc      m_on_recv{nullptr};
    OnTimeoutFunc   m_on_timeout{nullptr};
    OnConnectFunc   m_on_connect{nullptr};
    OnErrFunc       m_on_err{nullptr};
};

} // namespace bbt::network