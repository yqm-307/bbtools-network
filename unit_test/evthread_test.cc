#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include <bbt/network/adapter/libevent/Network.hpp>

typedef bbt::network::libevent::Network Network;
typedef bbt::network::libevent::Connection Connection;
typedef bbt::network::libevent::ConnectionSPtr ConnectionSPtr;
typedef bbt::network::Errcode Errcode;
typedef bbt::network::interface::INetConnectionSPtr INetConnectionSPtr;

BOOST_AUTO_TEST_SUITE(EventLoop_Test_Suit)

BOOST_AUTO_TEST_CASE(iothread_exit)
{
    std::atomic_int val = 0;
    bbt::network::libevent::IOThread evthread;    

    auto event = evthread.RegisterEvent(-1, bbt::network::libevent::EventOpt::PERSIST,
    [&](evutil_socket_t fd, short events){
        val++;  
    });

    event->StartListen(100);

    evthread.Start();

    while(1) {
        sleep(1);
        if (val >= 10) {
            evthread.Stop();
            break;
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()