#include <iostream>
#include <bbt/network/EvThread.hpp>
#include <bbt/core/clock/Clock.hpp>
#include <bbt/pollevent/Event.hpp>

using namespace bbt::network;

int main()
{

    auto evthread = std::make_shared<EvThread>(std::make_shared<bbt::pollevent::EventLoop>());

    // 开启一个定时器，每1s打印一次
    auto event = evthread->RegisterEvent(0, EventOpt::TIMEOUT | EventOpt::PERSIST, [](auto, auto events){
        Assert(events & EventOpt::TIMEOUT);
        printf("%shahhaha timeout了\n", bbt::core::clock::getnow_str().c_str());
    });

    Assert(event);

    event->StartListen(100);

    evthread->Start();
    evthread->Join();

}