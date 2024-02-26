/**
 * @file Network.hpp
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-23
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <bbt/network/adapter/libevent/Connection.hpp>
#include <bbt/network/adapter/base/Network.hpp>
#include <bbt/base/net/IPAddress.hpp>

namespace bbt::network::libevent
{

typedef std::function<void(libevent::ConnectionSPtr /* new_conn */)>    OnAcceptCallback;
typedef std::function<void(const Errcode& )>                            OnErrorCallback;

class Network:
    bbt::network::base::NetworkBase
{
public:
    Network(EventBase* io_context, const char* ip, short port);
    virtual ~Network();

    virtual std::pair<Errcode, interface::INetConnectionSPtr>
                                    AsyncConnect(const char* ip, short port);
    /* 初始化并设置监听事件 */
    Errcode                         Listen(const OnAcceptCallback& onaccept_cb);
    /* 设置network错误信息回调 */
    void                            SetOnErrorHandle(const OnErrorCallback& onerror_cb);
    /* 设置连接的io回调 */
    void                            SetConnectIOCallbacks(ConnCallbacks callbacks);    

protected:
    virtual std::pair<Errcode, interface::INetConnectionSPtr>  
                                    AsyncAccept();
    libevent::ConnectionSPtr        NewConn(int fd);    

    // virtual Errcode AsyncConnect(const char* ip, short port, const interface::OnConnectCallback& onconnect_cb) override;
    // virtual Errcode AsyncAccept(int listen_fd, const interface::OnAcceptCallback& onaccept_cb) override;

    libevent::ConnectionSPtr DoAccept(int listenfd);
    void OnError(const Errcode& err);
private:
    EventBase*                      m_io_context{nullptr};
    evutil_socket_t                 m_listen_fd{-1};            // network 监听套接字
    bbt::net::IPAddress             m_listen_addr;              // 等同服务器地址
    // interface::OnAcceptCallback     m_onaccept_handle{nullptr}; // 接收到新连接回调
    std::shared_ptr<Event>          m_onaccept_event{nullptr};
    OnErrorCallback                 m_error_handle{nullptr};

    ConnCallbacks                   m_conn_init_callbacks;      // 连接的io回调函数
};

} // namespace bbt::network::libevent