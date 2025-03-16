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
#include <bbt/network/EvThread.hpp>

namespace bbt::network
{

static const int io_thread_limit_num = 16;

IOThreadID EvThread::GenerateTid()
{
    static std::atomic_int _id = 0;
    return ++_id;
}


EvThread::EvThread(std::shared_ptr<EventLoop> evloop):
    m_eventloop(evloop)
{
    Init();
}

EvThread::~EvThread() 
{
    Destory();
}

void EvThread::Init()
{
    m_tid = GenerateTid();
}

void EvThread::Destory()
{
    m_tid = -1;
    delete m_thread;
    m_thread = nullptr;
}

IOThreadID EvThread::GetTid() const
{
    return m_tid;
}

std::thread::id EvThread::GetSysTid() const
{
    return m_thread->get_id();
}

void EvThread::StartWorkFunc()
{
    DebugAssertWithInfo(m_thread == nullptr,        "this is abnormal behavior");

    m_thread = new std::thread([=](){
        Work();
    });
}

void EvThread::Work()
{
    m_status = IOThreadRunStatus::Running;
    WorkHandle();
    m_status = IOThreadRunStatus::Finish;
}

bool EvThread::IsRunning() const
{
    return (m_status == IOThreadRunStatus::Running);
}

std::shared_ptr<Event> EvThread::RegisterEvent(evutil_socket_t fd, short events, const bbt::pollevent::OnEventCallback& onevent_cb)
{
    auto event_sptr = m_eventloop->CreateEvent(fd, events, onevent_cb);
    return event_sptr;
}

void EvThread::Join()
{
    m_thread->join();
}


void EvThread::Start()
{
    StartWorkFunc();
}

ErrOpt EvThread::Stop()
{
    if (m_status == Finish)
        return FASTERR_NOTHING;

    auto err = m_eventloop->BreakLoop();
    if (err != 0)
        return Errcode{"stop failed!", ERRTYPE_ERROR};

    /* 阻塞式的等待 */
    SyncWaitThreadExit();

    return FASTERR_NOTHING;
}

void EvThread::WorkHandle()
{
    auto err = m_eventloop->StartLoop(EVLOOP_NO_EXIT_ON_EMPTY);
    Assert(err == 0);
}

bool EvThread::SyncWaitThreadExitEx(int wait_time)
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

void EvThread::SyncWaitThreadExit()
{
    SyncWaitThreadExitEx(-1);
}

bool EvThread::SyncWaitThreadExitWithTime(int wait_time)
{
    return SyncWaitThreadExitEx(wait_time);
}



} // namespace bbt::network::detail