#pragma once
#include <bbt/network/detail/Define.hpp>

#include <functional>
#include <thread>
#include <atomic>
#include <bbt/pollevent/EventLoop.hpp>

namespace bbt::network
{

/* io线程id，小于0是非法值 */

class EvThread final:
    public std::enable_shared_from_this<EvThread>
{
public:
    EvThread(std::shared_ptr<EventLoop> evloop);
    virtual ~EvThread();

    void                    Start();
    ErrOpt                  Stop();
    IOThreadID              GetTid() const;
    bool                    IsRunning() const;
    std::shared_ptr<Event>  RegisterEvent(evutil_socket_t fd, short events, const bbt::pollevent::OnEventCallback& onevent_cb);
    void                    Join();
protected:
    virtual void            WorkHandle();
    std::thread::id         GetSysTid() const;
    void                    StartWorkFunc();
    void                    SyncWaitThreadExit();
    bool                    SyncWaitThreadExitWithTime(int wait_time);
    private:
    void                    Init();
    void                    Destory();
    void                    Work();
    bool                    SyncWaitThreadExitEx(int wait_time);
    static IOThreadID       GenerateTid();
private:
    std::shared_ptr<bbt::pollevent::EventLoop> m_eventloop{nullptr};
    std::thread*        m_thread{nullptr};
    IOThreadID          m_tid{-1};
    volatile bool       m_running{false};
    volatile IOThreadRunStatus   m_status{IOThreadRunStatus::Default};
};



} // namespace bbt::network::detail
