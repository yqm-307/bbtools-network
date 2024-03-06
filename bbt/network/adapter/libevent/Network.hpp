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
#include <bbt/network/adapter/libevent/IOThread.hpp>
#include <bbt/network/adapter/base/Network.hpp>
#include <bbt/base/net/IPAddress.hpp>

namespace bbt::network::libevent
{

enum NetworkStatus
{
    DEFAULT     = 0,
    STARTING    = 1,
    RUNNING     = 2,
    STOP        = 3,
};

typedef std::function<void(const Errcode&, libevent::ConnectionSPtr /* new_conn */)>    OnAcceptCallback;
typedef std::function<void(const Errcode&)>                                             OnNetworkErrorCallback;

class Network:
    bbt::network::base::NetworkBase
{
    typedef std::shared_ptr<IOThread> ThreadSPtr;
public:
    Network(uint32_t sub_thread_num);
    virtual ~Network();

    virtual Errcode                 AsyncConnect(const char* ip, short port, int timeout_ms, const interface::OnConnectCallback& onconnect_cb) override;

    /* 初始化并设置监听事件 */
    Errcode                         StartListen(const char* ip, short port, const OnAcceptCallback& onaccept_cb);
    /* 设置network错误信息回调 */
    void                            SetOnErrorHandle(const OnNetworkErrorCallback& onerror_cb);
    /* 开启所有线程 */
    void                            Start();
    /* 关闭所有线程 */
    void                            Stop();
    /* 获取 network 状态 */
    NetworkStatus                   Status();
protected:
    std::pair<Errcode, libevent::ConnectionSPtr>         
                                    DoAccept(int listenfd);
    void                            OnAccept(evutil_socket_t fd, short events, OnAcceptCallback cb);
    void                            OnError(const Errcode& err);
    Errcode                         DoConnect(evutil_socket_t fd, const bbt::net::IPAddress& addr);
    void                            OnConnect(
                                    std::shared_ptr<Event>  event,
                                    short                   events,
                                    bbt::timer::clock::Timestamp<bbt::timer::clock::ms> timeout,
                                    const bbt::net::IPAddress&  addr,
                                    interface::OnConnectCallback cb);

    ThreadSPtr                      GetAThread();

    void                            StopMainThread();
    void                            StopSubThread();
private:
    NetworkStatus                   m_status{NetworkStatus::DEFAULT};
    evutil_socket_t                 m_listen_fd{-1};            // network 监听套接字
    bbt::net::IPAddress             m_listen_addr;              // 等同服务器地址
    std::shared_ptr<Event>          m_onaccept_event{nullptr};
    OnNetworkErrorCallback          m_error_handle{nullptr};

    // ConnCallbacks                   m_conn_init_callbacks;      // 连接的io回调函数
    const int                       m_sub_loop_nums{1};         // 子（IO）线程数
    std::atomic_int                 m_cur_max_conn_count{0};    // 总接入连接数
    ThreadSPtr                      m_main_thread{nullptr};     // accept 线程
    std::vector<ThreadSPtr>         m_sub_threads;              // io 线程
    bbt::thread::lock::CountDownLatch*
                                    m_count_down_latch{nullptr};// 闭锁

private:
    struct ConnectEventMapImpl
    {
        Errcode                         AddConnectEvent(std::shared_ptr<Event> event);
        Errcode                         DelConnectEvent(std::shared_ptr<Event> event);
        std::mutex                      m_connect_mutex;
        std::map<EventId, std::shared_ptr<Event>>
                                        m_connect_events;           // 连接事件
    };

    ConnectEventMapImpl                 m_impl_connect_event_map;   // connect 事件集


};

} // namespace bbt::network::libevent