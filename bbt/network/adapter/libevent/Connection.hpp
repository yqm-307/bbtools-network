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
#include <bbt/base/buffer/Buffer.hpp>
#include <bbt/base/thread/Lock.hpp>
#include <bbt/network/Define.hpp>
#include <bbt/network/adapter/base/Connection.hpp>
#include <bbt/network/adapter/libevent/EventLoop.hpp>
#include <bbt/network/adapter/libevent/IOThread.hpp>

namespace bbt::network::libevent
{

class Connection;
typedef std::shared_ptr<Connection> ConnectionSPtr;


typedef std::function<void(ConnectionSPtr /*conn*/, const char* /*data*/, size_t /*len*/)> OnRecvCallback;
typedef std::function<void(const Errcode& /*err*/, size_t /*send_len*/)>    OnSendCallback;
typedef std::function<void()>                                               OnCloseCallback;
typedef std::function<void()>                                               OnTimeoutCallback;
typedef std::function<void(const Errcode&)>                                 OnErrorCallback;

struct ConnCallbacks
{
    OnRecvCallback      on_recv_callback{nullptr};
    OnSendCallback      on_send_callback{nullptr};
    OnCloseCallback     on_close_callback{nullptr};
    OnTimeoutCallback   on_timeout_callback{nullptr};
    OnErrorCallback     on_err_callback{nullptr};
};


class Connection:
    public base::ConnectionBase,
    public std::enable_shared_from_this<Connection>
{
    friend class bbt::templateutil::ManagerBase<ConnId, ConnectionBase>;
public:
    virtual ~Connection();


    /* 设置Connection的回调行为 */
    void                    SetOpt_Callbacks(const libevent::ConnCallbacks& callbacks);

    /* 设置空闲超时关闭Connection的时间 */
    void                    SetOpt_CloseTimeoutMS(int timeout_ms);

    /* 异步发送数据给对端 */
    int                     AsyncSend(const char* buf, size_t len);

    /* 关闭此连接 */
    virtual void            Close() override;

    /* 启动Connection */
    void                    RunInEventLoop();

private:
    Connection(
        std::shared_ptr<libevent::IOThread> thread,
        evutil_socket_t         socket,
        bbt::net::IPAddress&    ipaddr
    );
protected:
    void                    OnEvent(evutil_socket_t sockfd, short events);

    Errcode                 Recv(evutil_socket_t sockfd);
    size_t                  Send(const char* buf, size_t len);
    Errcode                 Timeout();

    virtual void            OnRecv(const char* data, size_t len) override;
    virtual void            OnSend(const Errcode& err, size_t succ_len) override;
    virtual void            OnClose() override;
    virtual void            OnTimeout() override;
    virtual void            OnError(const Errcode& err) override;

    int                     AsyncSendInThread();
    int                     AppendOutputBuffer(const char* data, size_t len);
private:
    std::shared_ptr<libevent::IOThread>
                            m_current_thread{nullptr};
    // std::shared_ptr<EventLoop>
    //                         m_eventloop{nullptr};       // io 上下文
    ConnCallbacks           m_callbacks;                // 回调函数
    std::shared_ptr<Event>  m_event{nullptr};           // 事件
    std::shared_ptr<Event>  m_send_event{nullptr};      // 发送事件
    uint64_t                m_conn_last_active_timestamp{0}; // 连接最后一次活跃（发送、接收）的时间戳

    /**
     * 异步写需要做输出缓存，这里策略是无限扩张的输出缓存。
     */
    bbt::buffer::Buffer     m_input_buffer;
    bbt::buffer::Buffer     m_output_buffer;
    bool                    m_output_buffer_is_free{true};
    bbt::thread::lock::Mutex
                            m_output_mutex;

    int                     m_timeout_ms{CONNECTION_FREE_TIMEOUT_MS};           // 连接空闲超时事件
    
};

}