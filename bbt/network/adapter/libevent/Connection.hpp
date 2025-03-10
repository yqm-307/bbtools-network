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
#include <bbt/network/Define.hpp>
#include <bbt/network/adapter/libevent/LibeventConnection.hpp>
#include <bbt/pollevent/EventLoop.hpp>

namespace bbt::network::libevent
{

class Connection;
typedef std::shared_ptr<Connection> ConnectionSPtr;


typedef std::function<void(ConnectionSPtr /*conn*/, const char* /*data*/, size_t /*len*/)> 
                                                                            OnRecvCallback;
typedef std::function<void(ConnectionSPtr /*conn*/, ErrOpt /*err */, size_t /*send_len*/)>   
                                                                            OnSendCallback;
typedef std::function<void(void* /*userdata*/, const IPAddress& )>OnCloseCallback;
typedef std::function<void(ConnectionSPtr /*conn*/)>                        OnTimeoutCallback;
typedef std::function<void(void* /*userdata*/, const Errcode&)>             OnConnErrorCallback;

struct ConnCallbacks
{
    OnRecvCallback      on_recv_callback{nullptr};
    OnSendCallback      on_send_callback{nullptr};
    OnCloseCallback     on_close_callback{nullptr};
    OnTimeoutCallback   on_timeout_callback{nullptr};
    OnConnErrorCallback     on_err_callback{nullptr};
};


class Connection:
    public libevent::LibeventConnection,
    public std::enable_shared_from_this<Connection>
{
    friend class libevent::IOThread;
    typedef bbt::pollevent::Event Event;
public:
    Connection(
        std::shared_ptr<libevent::IOThread> thread,
        evutil_socket_t                     socket,
        const IPAddress&          ipaddr
    );
    virtual ~Connection();


    static std::shared_ptr<Connection> Create(
        std::shared_ptr<libevent::IOThread> thread,
        evutil_socket_t                     socket,
        const IPAddress&          ipaddr
    );
    /* 设置Connection的回调行为 */
    void                    SetOpt_Callbacks(const libevent::ConnCallbacks& callbacks);
    /* 设置空闲超时关闭Connection的时间 */
    void                    SetOpt_CloseTimeoutMS(int timeout_ms);
    /* 设置用户数据 */
    void                    SetOpt_UserData(void* userdata);
    /* 读取用户数据 */
    void                    GetUserData(void* userdata);
    /* 异步发送数据给对端 */
    int                     AsyncSend(const char* buf, size_t len);
    /* 关闭此连接 */
    virtual void            Close() override;

protected:
    /* 启动Connection */
    void                    RunInEventLoop();
    void                    OnEvent(evutil_socket_t sockfd, short events);
    void                    OnSendEvent(std::shared_ptr<bbt::core::Buffer> output_buffer, std::shared_ptr<Event> event, short events);

    ErrOpt    Recv(evutil_socket_t sockfd);
    size_t                  Send(const char* buf, size_t len);
    ErrOpt    Timeout();

    virtual void            OnRecv(const char* data, size_t len) override;
    virtual void            OnSend(ErrOpt err, size_t succ_len) override;
    virtual void            OnClose() override;
    virtual void            OnTimeout() override;
    virtual void            OnError(const Errcode& err) override;

    int                     RegistASendEvent();
    int                     AppendOutputBuffer(const char* data, size_t len);
private:
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
    bbt::core::Buffer     m_output_buffer;
    std::atomic_bool        m_output_buffer_is_free{true}; // 是否被发送事件占用
    bbt::core::thread::Mutex
                            m_output_mutex;

    int                     m_timeout_ms{CONNECTION_FREE_TIMEOUT_MS};           // 连接空闲超时事件
    void*                   m_userdata{nullptr};    
};

}