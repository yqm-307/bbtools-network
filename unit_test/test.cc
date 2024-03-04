#include <bbt/network/adapter/libevent/Network.hpp>
#include <bbt/network/adapter/libevent/evThread.hpp>
#include <event2/thread.h>

typedef bbt::network::libevent::Network Network;
typedef bbt::network::libevent::Connection Connection;
typedef bbt::network::libevent::ConnectionSPtr ConnectionSPtr;
typedef bbt::network::Errcode Errcode;
typedef bbt::network::interface::INetConnectionSPtr INetConnectionSPtr;
using namespace bbt::network;

int main()
{
    Assert(bbt::network::GlobalInit());
    std::vector<std::shared_ptr<libevent::evThread>> vec;
    std::vector<std::shared_ptr<libevent::Event>> evec;

    for (int i = 0; i < 5; ++i){
        auto thread = std::make_shared<libevent::evThread>();
        auto event = thread->RegistEvent(-1, libevent::EventOpt::PERSIST | libevent::EventOpt::TIMEOUT, 
        [=](auto event, short){
        });
        event->StartListen(200);
        evec.push_back(event);
        vec.push_back(thread);
    }

    for (auto && it : vec) {
        it->Start();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}