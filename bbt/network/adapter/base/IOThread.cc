/**
 * @file IOThread.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "bbt/network/adapter/base/IOThread.hpp"

namespace bbt::network::base
{

static const int io_thread_limit_num = 16;

IOThreadID IOThread::GenerateTid()
{
    static std::atomic_int _id = 0;
    return ++_id;
}


IOThread::IOThread()
{
    Init();
}

IOThread::~IOThread() 
{
    Destory();
}

void IOThread::Init()
{
    m_tid = GenerateTid();
}

void IOThread::Destory()
{
    m_tid = -1;
    m_thread_start_before_callback = nullptr;
    m_thread_stop_after_callback = nullptr;
    delete m_thread;
    m_thread = nullptr;
}

IOThreadID IOThread::GetTid() const
{
    return m_tid;
}

std::thread::id IOThread::GetSysTid() const
{
    return m_thread->get_id();
}

void IOThread::StartWorkFunc()
{
    DebugAssertWithInfo(m_thread == nullptr,        "this is abnormal behavior");

    m_thread = new std::thread([=](){
        Work();
    });
}

void IOThread::SetOnThreadBegin_Hook(const HookCallback& cb)
{
    m_thread_start_before_callback = cb;
}
void IOThread::SetOnThreadEnd_Hook(const HookCallback& cb)
{
    m_thread_stop_after_callback = cb;
}

void IOThread::Work()
{
    /* 尝试执行线程开始前的 */
    if(m_thread_start_before_callback)
        m_thread_start_before_callback(m_tid);

    m_status = IOThreadRunStatus::Running;
    
    WorkHandle();

    m_status = IOThreadRunStatus::Finish;

    if(m_thread_stop_after_callback)
        m_thread_stop_after_callback(m_tid);
}

bool IOThread::IsRunning() const
{
    return (m_status == IOThreadRunStatus::Running);
}

void IOThread::Start()
{
    StartWorkFunc();
}

bool IOThread::SyncWaitThreadExitEx(int wait_time)
{
    const int interval = 50;    // 休眠间隔
    if(wait_time == 0)
        return false;
    int increase = wait_time > 0 ? interval : 0;
    int pass_time = 0;

    if(wait_time < 0 && m_thread->joinable())
        m_thread->join();
    
    while (m_thread->joinable() && pass_time < wait_time)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        pass_time += increase;    
    }

    return !(m_thread->joinable());
}

void IOThread::SyncWaitThreadExit()
{
    SyncWaitThreadExitEx(-1);
}

bool IOThread::SyncWaitThreadExitWithTime(int wait_time)
{
    return SyncWaitThreadExitEx(wait_time);
}



}