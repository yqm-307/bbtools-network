/**
 * @file Connection.hpp
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <bbt/core/buffer/Buffer.hpp>
#include <bbt/core/thread/Lock.hpp>
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/network/detail/Define.hpp>

namespace bbt::network::detail
{

class Connection:
    public std::enable_shared_from_this<Connection>,
    boost::noncopyable
{
    friend class EvThread;
public:
    BBTATTR_FUNC_CTOR_HIDDEN Connection(
        std::weak_ptr<EvThread> thread,
        evutil_socket_t           socket,
        const IPAddress&          ipaddr
    );

    virtual ~Connection();


    static std::shared_ptr<Connection> Create(
        std::weak_ptr<EvThread> thread,
        evutil_socket_t           socket,
        const IPAddress&          ipaddr
    );
    /* 设置Connection的回调行为 */
    void                    SetOpt_Callbacks(const ConnCallbacks& callbacks);
    /* 设置空闲超时关闭Connection的时间 */
    void                    SetOpt_CloseTimeoutMS(int timeout_ms);
    /* 异步发送数据给对端 */
    core::errcode::ErrOpt   AsyncSend(const char* buf, size_t len);
    /* 关闭此连接 */
    void                    Close();
    bool                    IsConnected() const;
    bool                    IsClosed() const;
    const IPAddress&        GetPeerAddress() const;
    evutil_socket_t         GetSocket() const;
    ConnId                  GetConnId() const;
    void                    RunInEventLoop();

protected:
    /* 启动Connection */
    void                    OnEvent(evutil_socket_t sockfd, short events);
    void                    OnSendEvent(std::shared_ptr<bbt::core::Buffer> output_buffer, short events);

    core::errcode::ErrOpt   Recv(evutil_socket_t sockfd);
    size_t                  Send(const char* buf, size_t len);
    core::errcode::ErrOpt   Timeout();

    void                    OnRecv(const char* data, size_t len);
    void                    OnSend(core::errcode::ErrOpt err, size_t succ_len);
    void                    OnClose();
    void                    OnTimeout();
    void                    OnError(const core::errcode::Errcode& err);

    core::errcode::ErrOpt   RegistASendEvent();
    int                     AppendOutputBuffer(const char* data, size_t len);

    std::shared_ptr<EvThread> GetBindThread();
    bool                    BindThreadIsRunning();

    virtual void            CloseSocket() final; 
    virtual void            SetStatus(ConnStatus status) final;
    static ConnId           GenerateConnId();
private:
    std::weak_ptr<EvThread> m_bind_thread;

    ConnCallbacks           m_callbacks;                // 回调函数
    /**
     * 一连接一事件，
     * 1、如果使用多个事件会有时序问题
     * 2、连接内部派发对于事件处理函数，更简洁
     */
    std::shared_ptr<Event>  m_event{nullptr};           // 事件
    std::shared_ptr<Event>  m_send_event{nullptr};      // 发送事件

    /**
     * 异步写需要做输出缓存，这里策略是无限扩张的输出缓存。
     */
    bbt::core::Buffer       m_output_buffer;
    std::atomic_bool        m_output_buffer_is_free{true}; // 是否被发送事件占用
    bbt::core::thread::Mutex
                            m_output_mutex;

    int                     m_timeout_ms{CONNECTION_FREE_TIMEOUT_MS};           // 连接空闲超时事件

    int                     m_socket_fd{-1};
    IPAddress               m_peer_addr;
    volatile ConnStatus     m_conn_status{ConnStatus::emCONN_DEFAULT};
    const ConnId            m_conn_id{0};
};

}