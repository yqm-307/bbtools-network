#pragma once
#include <bbt/network/Define.hpp>
#include <bbt/core/crypto/BKDR.hpp>

namespace bbt::network
{

class TcpServer final:
    public std::enable_shared_from_this<TcpServer>
{
public:
    TcpServer(std::shared_ptr<EvThread> evthread);
    ~TcpServer();

    void            Init();

    ErrOpt          AsyncListen(const bbt::core::net::IPAddress& addr, const OnAcceptFunc& onaccept_cb);
    ErrOpt          StopListen();
    IPAddress       GetListenAddress();
    bool            IsListening();
    void            SetTimeout(int connection_timeout);
    ErrOpt          Send(ConnId connid, const bbt::core::Buffer& buffer);
    void            Close(ConnId connid);

    void            SetOnTimeout(const OnTimeoutFunc& on_timeout) { m_on_timeout = on_timeout; }
    void            SetOnClose(const OnCloseFunc& on_close) { m_on_close = on_close; }
    void            SetOnSend(const OnSendFunc& on_send) { m_on_send = on_send; }
    void            SetOnRecv(const OnRecvFunc& on_recv) { m_on_recv = on_recv; }
    void            SetOnErr(const OnErrFunc& on_err) { m_on_err = on_err; }

private:
    void            OnTimeout(ConnId connid);
    void            OnClose(ConnId connid);
    void            OnSend(ConnId connid, ErrOpt err, size_t send_len);
    void            OnRecv(ConnId connid, bbt::core::Buffer& buffer);

    std::shared_ptr<EvThread> GetThread();
    void            _Accept(int fd, short events, const OnAcceptFunc& onaccept_cb, std::shared_ptr<EvThread> thread);
    void            _InitConnection(std::shared_ptr<detail::Connection> conn);

    struct ConnectEventMapImpl;
    struct AddressHash { std::size_t operator()(const IPAddress& addr) const { return core::crypto::BKDR::BKDRHash(addr.GetIPPort());}; };

private:
    std::shared_ptr<EvThread>       m_ev_thread{nullptr};
    detail::ConnCallbacks           callbacks;

    std::unordered_map<ConnId, detail::ConnectionSPtr> m_conn_map;
    std::mutex                      m_conn_map_mutex;

    IPAddress                       m_listen_addr;
    std::shared_ptr<Event>          m_listen_event{nullptr};
    std::mutex                      m_listen_mtx;

    int                             m_connection_timeout{10000};

    OnTimeoutFunc   m_on_timeout{nullptr};
    OnCloseFunc     m_on_close{nullptr};
    OnSendFunc      m_on_send{nullptr};
    OnRecvFunc      m_on_recv{nullptr};
    OnErrFunc       m_on_err{nullptr};
};

} // namespace bbt::network