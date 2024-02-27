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

namespace bbt::network::libevent
{

class Connection;
class Network;
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
    friend class Network;
public:
    virtual ~Connection();

    /* 关闭此连接 */
    virtual void            Close() override;
private:
    Connection(
        EventLoop*              eventloop,
        evutil_socket_t         socket,
        bbt::net::IPAddress&    ipaddr
    );


    /* 在libevent中注册事件 */
    void                    RegistEvent();
    /* 在libevent中注销事件 */
    void                    UnRegistEvent();
    void                    SetCallbacks(const libevent::ConnCallbacks& cb);
    int                     AsyncSend(const char* buf, size_t len);
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
    std::shared_ptr<EventLoop>
                            m_eventloop{nullptr};       // io 上下文
    ConnCallbacks           m_callbacks;                // 回调函数
    std::shared_ptr<Event>  m_event{nullptr};           // 事件
    std::shared_ptr<Event>  m_send_event{nullptr};      // 发送事件

    /**
     * 异步写需要做输出缓存，这里策略是无限扩张的输出缓存。
     */
    bbt::buffer::Buffer     m_input_buffer;
    bbt::buffer::Buffer     m_output_buffer;
    bool                    m_output_buffer_is_free{true};
    bbt::thread::lock::Mutex    m_output_mutex;
    
};

}