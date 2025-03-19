#include <iostream>
#include <bbt/network/EvThread.hpp>
#include <bbt/core/clock/Clock.hpp>
#include <bbt/pollevent/Event.hpp>
#include <bbt/core/thread/Lock.hpp>
#include <sys/signal.h>

using namespace bbt::network;

int main()
{
    bbt::core::thread::CountDownLatch l{1};
    auto evthread = std::make_shared<EvThread>(std::make_shared<bbt::pollevent::EventLoop>());

    // ctrl z 事件监听
    auto event_sigint = evthread->RegisterEvent(SIGINT, EventOpt::SIGNAL | EventOpt::PERSIST,
    [evthread, &l](auto, short events, auto){
        // std::cout << "[EvThread] ctrl z" << std::endl;
        // evthread->Stop();
        l.Down();
    });

    event_sigint->StartListen(0);

    // 开启一个定时器，每1s打印一次
    auto event = evthread->RegisterEvent(0, EventOpt::TIMEOUT | EventOpt::PERSIST, [](auto, auto events, auto){
        Assert(events & EventOpt::TIMEOUT);
        printf("%shahhaha timeout了\n", bbt::core::clock::getnow_str().c_str());
    });

    Assert(event);

    event->StartListen(100);

    evthread->Start();
    // evthread->Join();
    l.Wait();
    evthread->Stop();

}