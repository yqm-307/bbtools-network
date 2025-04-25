#pragma once
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/network/detail/Define.hpp>
#include <bbt/core/crypto/BKDR.hpp>

namespace bbt::network
{

class TcpServer final:
    public std::enable_shared_from_this<TcpServer>
{
public:
    TcpServer(std::shared_ptr<EvThread> evthread);
    TcpServer(int nthread);
    TcpServer(const std::vector<std::shared_ptr<EvThread>>& evthreads);
    ~TcpServer();

    /**
     * @brief 初始化内部事件
     */
    void            Init();

    /**
     * @brief 启动监听
     * 
     * @param addr 监听的地址
     * @param onaccept_cb 接受连接的回调函数
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt AsyncListen(const bbt::core::net::IPAddress& addr, const OnAcceptFunc& onaccept_cb);

    /**
     * @brief 停止监听
     * 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt StopListen();

    /**
     * @brief 获取当前监听的地址
     * 
     * @return IPAddress 
     */
    IPAddress       GetListenAddress();

    /**
     * @brief 是否正在监听
     * 
     * @return true 
     * @return false 
     */
    bool            IsListening();

    /**
     * @brief 设置一个超时时间
     * 在TcpServer接收到一个新的连接后，会设置一个超时时间
     * 当连接空闲超过connection_timeout后，则会触发Timeout
     * 并关闭连接
     * 
     * @param connection_timeout 
     */
    void            SetTimeout(int connection_timeout);

    /**
     * @brief 向指定的连接发送数据，这个接口是异步且线程安全的
     * 
     * @param connid 
     * @param buffer 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt Send(ConnId connid, const bbt::core::Buffer& buffer);

    /**
     * @brief 关闭指定连接
     * 
     * @param connid 
     */
    void            Close(ConnId connid);

    // 设置回调
    void            SetOnTimeout(const OnTimeoutFunc& on_timeout) { m_on_timeout = on_timeout; }
    void            SetOnClose(const OnCloseFunc& on_close) { m_on_close = on_close; }
    void            SetOnSend(const OnSendFunc& on_send) { m_on_send = on_send; }
    void            SetOnRecv(const OnRecvFunc& on_recv) { m_on_recv = on_recv; }
    void            SetOnErr(const OnErrFunc& on_err) { m_on_err = on_err; }

private:
    void            OnTimeout(ConnId connid);
    void            OnClose(ConnId connid);
    void            OnSend(ConnId connid, core::errcode::ErrOpt err, size_t send_len);
    void            OnRecv(ConnId connid, bbt::core::Buffer& buffer);

    std::shared_ptr<EvThread> GetThread();
    void            _Accept(int fd, short events, const OnAcceptFunc& onaccept_cb, std::shared_ptr<EvThread> thread);
    void            _InitConnection(std::shared_ptr<detail::Connection> conn);

    struct ConnectEventMapImpl;
    struct AddressHash { std::size_t operator()(const IPAddress& addr) const { return core::crypto::BKDR::BKDRHash(addr.GetIPPort());}; };

private:
    std::vector<pollevent::EvThread::SPtr>          m_thread_pool;
    const size_t                                    m_thread_count{0};
    uint8_t                                         m_load_blance{0};

    detail::ConnCallbacks           callbacks;

    std::unordered_map<ConnId, detail::ConnectionSPtr> m_conn_map;
    std::mutex                      m_conn_map_mutex;

    IPAddress                       m_listen_addr;
    int                             m_listen_fd{-1};
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