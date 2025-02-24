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

enum IOThreadType : int
{
    LISTENANDCONNECT        = 0,    // 监听线程
    IO                      = 1,    // IO线程

    CUSTOM                  = 10,   // 从此开始都是自定义
};

typedef std::function<void(const Errcode&, libevent::ConnectionSPtr /* new_conn */)>    OnAcceptCallback;
typedef std::function<void(const Errcode&)>                                             OnNetworkErrorCallback;

class Network final:
    bbt::network::base::NetworkBase
{
    typedef std::shared_ptr<libevent::IOThread> ThreadSPtr;
    typedef bbt::pollevent::Event Event;
    typedef bbt::pollevent::EventLoop EventLoop;

public:
    Network();
    virtual ~Network();

    virtual Errcode                 AsyncConnect(const char* ip, short port, int timeout_ms, const interface::OnConnectCallback& onconnect_cb) override;

    /* 初始化并设置监听事件 */
    virtual Errcode                 StartListen(const char* ip, short port, const interface::OnAcceptCallback& onaccept_cb) override;

    /* 启动Network */
    void                            Start() override;

    /* 停止Network */
    void                            Stop() override;

    /**
     * @brief 获取network的状态
     * 
     * @return NetworkStatus 
     */
    NetworkStatus                   Status() override;

    /**
     * @brief 添加一个IOThread
     * 
     * @param type 
     * @param thread 
     * @return Errcode 
     */
    Errcode                         AddIOThread(IOThreadType type, std::shared_ptr<libevent::IOThread> thread);

    /**
     * @brief 自动创建主线程和sub_thread_num数量的IO线程
     * 
     * @param sub_thread_num 
     * @return Errcode 
     */
    Errcode                         AutoInitThread(int sub_thread_num);

    ThreadSPtr                      GetAIOThread();
protected:
    std::pair<ThreadSPtr, Errcode>  GetThread(IOThreadType type);
    ThreadSPtr                      GetListenAndConnectThread();

    void                            StopMainThread();
    void                            StopSubThread();
private:
    std::unordered_multimap<int, std::shared_ptr<libevent::IOThread>> 
                                    m_thread_map;

    NetworkStatus                   m_status{NetworkStatus::emNETWORK_DEFAULT};
    bbt::thread::CountDownLatch*
                                    m_count_down_latch{nullptr};// 闭锁
};

} // namespace bbt::network::libevent