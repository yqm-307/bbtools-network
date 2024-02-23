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
#include <bbt/network/Define.hpp>
#include <bbt/network/adapter/base/Connection.hpp>
#include <bbt/network/adapter/libevent/Event.hpp>

namespace bbt::network::libevent
{

class Connection;
class Network;
typedef std::shared_ptr<Connection> ConnectionSPtr;


typedef std::function<void(ConnectionSPtr /*conn*/, const char* /*data*/, size_t /*len*/)> OnRecvCallback;
typedef std::function<void(const Errcode& /*err*/, size_t /*send_len*/)>   OnSendCallback;
typedef std::function<void()>                                       OnCloseCallback;
typedef std::function<void()>                                       OnTimeoutCallback;
typedef std::function<void(const Errcode&)>                         OnErrorCallback;

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
    Connection(
        EventBase*              base,
        evutil_socket_t         socket,
        bbt::net::IPAddress&    ipaddr,
        const ConnCallbacks&    callbacks);

    ~Connection();

    /* 关闭此连接 */
    virtual void        Close() override;
private:
    /* 在libevent中注册事件 */
    void                RegistEvent();
    /* 在libevent中注销事件 */
    void                UnRegistEvent();
protected:
    virtual void        OnRecv(const char* data, size_t len) override;
    virtual void        OnSend(const Errcode& err, size_t succ_len) override;
    virtual void        OnClose() override;
    virtual void        OnTimeout() override;
    virtual void        OnError(const Errcode& err) override;
private:
    EventBase*              m_io_context;   // io 上下文
    ConnCallbacks           m_callbacks;    // 回调函数
    std::shared_ptr<Event>  m_event;        // 事件
};

}