#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include <bbt/network/adapter/libevent/Network.hpp>
#include <bbt/network/adapter/libevent/evThread.hpp>
#include <event2/thread.h>

typedef bbt::network::libevent::Network Network;
typedef bbt::network::libevent::Connection Connection;
typedef bbt::network::libevent::ConnectionSPtr ConnectionSPtr;
typedef bbt::network::Errcode Errcode;
typedef bbt::network::interface::INetConnectionSPtr INetConnectionSPtr;
using namespace bbt::network;

BOOST_AUTO_TEST_SUITE(EventLoop_Test_Suit)

BOOST_AUTO_TEST_CASE(iothread_exit)
{
    // std::atomic_int val = 0;
    // bbt::network::libevent::IOThread evthread;    

    // auto event = evthread.RegisterEvent(-1, bbt::network::libevent::EventOpt::PERSIST,
    // [&](evutil_socket_t fd, short events){
    //     val++;
    // });

    // event->StartListen(100);

    // evthread.Start();

    // sleep(1);
    // evthread.Stop();
}

BOOST_AUTO_TEST_CASE(evthread_exit)
{
    Assert(bbt::network::GlobalInit());
    std::vector<std::shared_ptr<libevent::evThread>> vec;
    std::vector<std::shared_ptr<libevent::Event>> evec;

    for (int i = 0; i < 5; ++i){
        auto thread = std::make_shared<libevent::evThread>();
        auto event = thread->RegistEvent(-1, libevent::EventOpt::PERSIST | libevent::EventOpt::TIMEOUT, [=](int, short){
            printf("%d\n", i);
        });
        event->StartListen(200);
        evec.push_back(event);
        vec.push_back(thread);
    }

    for (auto && it : vec) {
        it->Start();
    }
    sleep(1);

    for (auto && event : evec) {
        event->CancelListen();
    }
}

BOOST_AUTO_TEST_CASE(multi_iothread_exit)
{
    // Assert(evthread_use_pthreads() == 0);
    // std::vector<std::shared_ptr<libevent::IOThread>> vec;
    // std::vector<std::shared_ptr<libevent::Event>> evec;

    // for (int i = 0; i < 5; ++i){
    //     auto thread = std::make_shared<libevent::IOThread>();
    //     auto event = thread->RegisterEvent(-1, libevent::EventOpt::PERSIST, [=](int, short){
    //         printf("%d\n", i);
    //     });
    //     event->StartListen(500);
    //     evec.push_back(event);
    //     vec.push_back(thread);
    // }

    // for (auto && it : vec) {
    //     it->Start();
    // }
    // sleep(1);
}

BOOST_AUTO_TEST_SUITE_END()