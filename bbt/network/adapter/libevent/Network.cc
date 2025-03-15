/**
 * @file Network.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <bbt/core/net/SocketUtil.hpp>
#include <bbt/network/adapter/libevent/Network.hpp>
#include <bbt/core/crypto/Random.hpp>
#include <event2/util.h>

namespace bbt::network::libevent
{

void WaitForCountDown(bbt::core::thread::CountDownLatch* latch)
{
    latch->Down();
    latch->Wait();
}

Network::Network()
{
}

Network::~Network()
{
    delete m_count_down_latch;
    m_count_down_latch = nullptr;

    if (Status() == NetworkStatus::emNETWORK_RUNNING) {
        Stop();
    }

}


void Network::Start()
{
    if (m_status != NetworkStatus::emNETWORK_DEFAULT)
        return;

    m_status = NetworkStatus::emNETWORK_STARTING;
    m_count_down_latch = new bbt::core::thread::CountDownLatch(m_thread_map.size());

    // 启动子线程
    for (auto&& itor : m_thread_map) {
        itor.second->SetOnStart([=](auto){
            m_count_down_latch->Down();
        });
    }

    for (auto&& thread : m_thread_map)
        thread.second->Start();

    m_count_down_latch->Wait();
    m_status = NetworkStatus::emNETWORK_RUNNING;
}

void Network::Stop()
{
    if (m_status != NetworkStatus::emNETWORK_RUNNING)
        return;

    

    /*停止接收线程*/
    StopMainThread();

    /* 停止IO线程 */
    StopSubThread();

    m_status = NetworkStatus::emNETWORK_STOP;
}

ErrOpt Network::AddIOThread(IOThreadType type, std::shared_ptr<libevent::IOThread> thread)
{

    if (thread == nullptr)
        return FASTERR_ERROR("thread is null!");

    switch (type)
    {
    case IOThreadType::LISTENANDCONNECT :
        if (m_thread_map.count(IOThreadType::LISTENANDCONNECT) >= 1)
            return FASTERR_ERROR("listen thread can`t be more than 1!");
        m_thread_map.insert(std::make_pair(IOThreadType::LISTENANDCONNECT, thread));
        break;

    case IOThreadType::IO :
        m_thread_map.insert(std::make_pair(IOThreadType::IO, thread));
        break;

    default:
        if (type < 0)
            return FASTERR_ERROR("type less then 0!");
        else if (type < IOThreadType::CUSTOM)
            return FASTERR_ERROR("nodefine thread type!");
        else
            m_thread_map.insert(std::make_pair(type, thread));
        break;
    }

    return FASTERR_NOTHING;
}

ErrOpt Network::AutoInitThread(int sub_thread_num)
{
    if (m_thread_map.size() != 0)
        return FASTERR_ERROR("thread is exist! please clear network thread!"); // 已经通过其他方式初始化过线程了
    
    auto eventloop = std::make_shared<EventLoop>();
    auto thread = std::make_shared<libevent::IOThread>(eventloop);
    AddIOThread(IOThreadType::LISTENANDCONNECT, thread);

    for (int i = 0; i < sub_thread_num; ++i) {
        eventloop = std::make_shared<EventLoop>();
        thread = std::make_shared<libevent::IOThread>(eventloop);
        AddIOThread(IOThreadType::IO, thread);
    }

    return FASTERR_NOTHING;
}

void Network::StopMainThread()
{
    auto listen_and_connect_thread = GetListenAndConnectThread();
    if (listen_and_connect_thread->IsRunning()) {
        Assert(!listen_and_connect_thread->Stop().has_value());
    }

    m_thread_map.erase(IOThreadType::LISTENANDCONNECT);
}

void Network::StopSubThread()
{
    for (auto&& itor : m_thread_map) {
        auto thread = itor.second;
        if (!thread->IsRunning())
            Assert(!thread->Stop().has_value());
    }

    m_status = NetworkStatus::emNETWORK_STOP;
}

ErrOpt Network::StartListen(const char* ip, short port, const interface::IOnAcceptCallback& onaccept_cb)
{
    auto listen_and_connect_thread = GetListenAndConnectThread();
    if (listen_and_connect_thread == nullptr)
        return FASTERR_ERROR("listen thread is null!");

    return listen_and_connect_thread->Listen(ip, port, onaccept_cb, GetAIOThread());
}

NetworkStatus Network::Status()
{
    return m_status;
}

Network::ThreadSPtr Network::GetAIOThread()
{
    auto [thread, err] = GetThread(IOThreadType::IO);

    return thread;
}

Network::ThreadSPtr Network::GetListenAndConnectThread()
{
    auto [thread, _] = GetThread(IOThreadType::LISTENANDCONNECT);

    return thread;
}


std::pair<Network::ThreadSPtr, ErrOpt> Network::GetThread(IOThreadType type)
{
    static bbt::core::crypto::mt_random<> rd;
    if (type < 0) {
        return {nullptr, FASTERR_ERROR("bad thread type!")};
    }

    if (type == IOThreadType::LISTENANDCONNECT) {
    // Listen Connect 现成限制只能有一个
        auto count = m_thread_map.count(type);
        if (count != 1) 
            return {nullptr, FASTERR_ERROR("listen and connect thread num bad! count=" + std::to_string(count))};
        
        return {m_thread_map.find(type)->second, FASTERR_NOTHING}; // 只有一个item，则返回唯一的item
    } else {
    // 其他类型线程不限制数量
        auto count = m_thread_map.count(type);
        if (count <= 0)
            return {nullptr, FASTERR_ERROR("can`t found thread! type=" + std::to_string(type))};

        // 从一类threads中随机选择一个
        auto rand_index = rd() % count;
        auto itor_pair = m_thread_map.equal_range(type);
        auto itor_begin = itor_pair.first;
        for (int i = 0; i < rand_index; ++i) itor_begin++;

        return {itor_begin->second, FASTERR_NOTHING}; // 随机返回一个
    }
}


ErrOpt Network::AsyncConnect(const char* ip, short port, int timeout_ms, const interface::IOnConnectCallback& onconnect)
{
    auto listen_and_connect_thread = GetListenAndConnectThread();

    if (listen_and_connect_thread == nullptr)
        return FASTERR_ERROR("listen thread is null!");


    return listen_and_connect_thread->AsyncConnect(ip, port, timeout_ms, onconnect);
}

} // namespace bbt::network::libevent