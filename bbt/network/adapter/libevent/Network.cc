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
#include <bbt/base/net/SocketUtil.hpp>
#include <bbt/network/adapter/libevent/Network.hpp>
#include <bbt/base/random/Random.hpp>
#include <event2/util.h>

namespace bbt::network::libevent
{

void WaitForCountDown(bbt::thread::lock::CountDownLatch* latch)
{
    latch->down();
    latch->wait();
}

Network::Network()
{
}

Network::~Network()
{
    delete m_count_down_latch;
    m_count_down_latch = nullptr;

    if (Status() == NetworkStatus::RUNNING) {
        Stop();
    }

}


void Network::Start()
{
    if (m_status != NetworkStatus::DEFAULT)
        return;

    m_status = NetworkStatus::STARTING;
    m_count_down_latch = new bbt::thread::lock::CountDownLatch(m_thread_map.size());

    // 启动子线程
    for (auto&& itor : m_thread_map) {
        itor.second->SetOnStart([=](auto){
            m_count_down_latch->down();
        });
    }

    for (auto&& thread : m_thread_map)
        thread.second->Start();

    m_count_down_latch->wait();
    m_status = NetworkStatus::RUNNING;
}

void Network::Stop()
{
    if (m_status != NetworkStatus::RUNNING)
        return;

    

    /*停止接收线程*/
    StopMainThread();

    /* 停止IO线程 */
    StopSubThread();

    m_status = NetworkStatus::STOP;
}

Errcode Network::AddIOThread(IOThreadType type, std::shared_ptr<libevent::IOThread> thread)
{

    if (thread == nullptr)
        return Errcode{"thread is null!"};

    switch (type)
    {
    case IOThreadType::LISTENANDCONNECT :
        if (m_thread_map.count(IOThreadType::LISTENANDCONNECT) >= 1)
            return Errcode{"listen thread can`t be more than 1!"};
        m_thread_map.insert(std::make_pair(IOThreadType::LISTENANDCONNECT, thread));
        break;

    case IOThreadType::IO :
        m_thread_map.insert(std::make_pair(IOThreadType::IO, thread));
        break;

    default:
        if (type < 0)
            return Errcode{"type less then 0!"};
        else if (type < IOThreadType::CUSTOM)
            return Errcode{"nodefine thread type!"};
        else
            m_thread_map.insert(std::make_pair(type, thread));
        break;
    }

    return FASTERR_NOTHING;
}

Errcode Network::AutoInitThread(int sub_thread_num)
{
    if (m_thread_map.size() != 0)
        return Errcode{"thread is exist! please clear network thread!"}; // 已经通过其他方式初始化过线程了
    
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
        Assert(listen_and_connect_thread->Stop());
    }

    m_thread_map.erase(IOThreadType::LISTENANDCONNECT);
}

void Network::StopSubThread()
{
    for (auto&& itor : m_thread_map) {
        auto thread = itor.second;
        if (!thread->IsRunning())
            Assert(thread->Stop());
    }

    m_status = NetworkStatus::STOP;
}

Errcode Network::StartListen(const char* ip, short port, const OnAcceptCallback& onaccept_cb)
{
    auto listen_and_connect_thread = GetListenAndConnectThread();
    if (listen_and_connect_thread == nullptr)
        return Errcode{"listen thread is null!"};

    return listen_and_connect_thread->Listen(ip, port, onaccept_cb);
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


std::pair<Network::ThreadSPtr, Errcode> Network::GetThread(IOThreadType type)
{
    static bbt::random::mt_random<> rd;
    if (type < 0) {
        return {nullptr, Errcode{"bad thread type!"}};
    }

    if (type == IOThreadType::LISTENANDCONNECT) {
    // Listen Connect 现成限制只能有一个
        auto count = m_thread_map.count(type);
        if (count != 1) 
            return {nullptr, Errcode{"listen and connect thread num bad! count=" + std::to_string(count)}};
        
        return {m_thread_map.find(type)->second, FASTERR_NOTHING}; // 只有一个item，则返回唯一的item
    } else {
    // 其他类型线程不限制数量
        auto count = m_thread_map.count(type);
        if (count <= 0)
            return {nullptr, Errcode{"can`t found thread! type=" + std::to_string(type)}};

        // 从一类threads中随机选择一个
        auto rand_index = rd() % count;
        auto itor_pair = m_thread_map.equal_range(type);
        auto itor_begin = itor_pair.first;
        for (int i = 0; i < rand_index; ++i) itor_begin++;

        return {itor_begin->second, FASTERR_NOTHING}; // 随机返回一个
    }
}


Errcode Network::AsyncConnect(const char* ip, short port, int timeout_ms, const interface::OnConnectCallback& onconnect)
{
    auto listen_and_connect_thread = GetListenAndConnectThread();

    if (listen_and_connect_thread == nullptr)
        return Errcode{"listen thread is null!"};


    return listen_and_connect_thread->AsyncConnect(ip, port, timeout_ms, onconnect);
}

} // namespace bbt::network::libevent