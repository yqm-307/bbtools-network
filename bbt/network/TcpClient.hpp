#pragma once
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/network/detail/Define.hpp>

namespace bbt::network
{

class TcpClient final:
    public std::enable_shared_from_this<TcpClient>
{
public:
    TcpClient(std::shared_ptr<pollevent::EvThread> evthread);
    ~TcpClient() = default;

    /**
     * @brief 初始化TcpClient的内部事件
     */
    void            Init();

    /**
     * @brief 向addr发起一个异步连接，且当连接超过timeout时间还未
     * 成功建立，则连接失败。无论成功或者失败，都会通过OnConnect回
     * 调通知
     * 
     * @param addr 
     * @param timeout 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt AsyncConnect(const bbt::core::net::IPAddress& addr, int timeout);

    core::errcode::ErrOpt Connect(const bbt::core::net::IPAddress& addr, int timeout);

    /**
     * @brief 重新发起连接
     * 
     * 实际上就是调用了一次AsyncConnect，但是使用最近一次AsyncConnect的地址和设置
     * 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt ReConnect();

    /**
     * @brief 向对端发送数据，这个接口是异步且线程安全的
     * 
     * @param buffer 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt Send(const bbt::core::Buffer& buffer);

    /**
     * @brief 关闭连接
     * 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt Close();

    /**
     * @brief 判断当前连接是否进行中
     * 
     * @return true 
     * @return false 
     */
    bool            IsConnected();

    /**
     * @brief 获取当前TcpClient的连接id
     * 
     * @return ConnId 成功返回大于0的值，如果失败返回-1
     */
    ConnId          GetConnId();

    void            SetConnectionTimeout(int timeout) { m_connection_timeout = timeout; }
    void            SetOnConnect(const OnConnectFunc& on_connect) { m_on_connect = on_connect; }
    void            SetOnTimeout(const OnTimeoutFunc& on_timeout) { m_on_timeout = on_timeout; }
    void            SetOnClose(const OnCloseFunc& on_close) { m_on_close = on_close; }
    void            SetOnSend(const OnSendFunc& on_send) { m_on_send = on_send; }
    void            SetOnRecv(const OnRecvFunc& on_recv) { m_on_recv = on_recv; }
    void            SetOnErr(const OnErrFunc& on_err) {m_on_err = on_err; }
private:
    std::shared_ptr<pollevent::EvThread> _GetThread();
    void            _DoConnect(int socket, short events);
    void            _DoConnectThreadSafe(int socket, short events);
    void            _InitConnection(std::shared_ptr<detail::Connection> conn);
    void            _OnClose(ConnId id);
private:
    std::shared_ptr<pollevent::EvThread> m_ev_thread{nullptr};

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