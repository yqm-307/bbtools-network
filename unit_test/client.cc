#include <event2/thread.h>
#include <bbt/network/adapter/libevent/Network.hpp>
#include <bbt/core/clock/Clock.hpp>

typedef bbt::network::libevent::Network Network;
typedef bbt::network::libevent::Connection Connection;
typedef bbt::network::libevent::ConnectionSPtr ConnectionSPtr;
typedef bbt::network::interface::INetConnectionSPtr INetConnectionSPtr;

using namespace bbt::core::errcode;
using namespace bbt::network;

int main()
{
    Network network;
    network.AutoInitThread(1);
    std::vector<ConnectionSPtr> conn_vec;

    network.AsyncConnect("127.0.0.1", 10010, 1000,
    [&network, &conn_vec](ErrOpt err, INetConnectionSPtr i_sptr){
        if (err.has_value()) {
            printf("connect err! %s\n", err->CWhat());
            return;
        }
        auto ptr = std::static_pointer_cast<Connection>(i_sptr);
        bbt::network::libevent::ConnCallbacks callback;
        callback.on_err_callback = [](auto, const Errcode& err){
            printf("[onerr] %s %ld\n", err.CWhat(), bbt::core::clock::gettime());
        };
        callback.on_timeout_callback = [](auto conn){
            printf("[ontimeout] %ld\n", bbt::core::clock::gettime());
            conn->Close();
        };
        callback.on_close_callback = [](void*, const IPAddress& addr){
            printf("[onclose] %s %ld\n", addr.GetIPPort().c_str(), bbt::core::clock::gettime());
        };

        ptr->SetOpt_Callbacks(callback);
        conn_vec.push_back(ptr);
        printf("connect success!\n");
    });

    network.Start();
    // 防止主线程退出
    for (int i = 0; i < 2; ++i)
        sleep(5);
    printf("2\n");
    network.Stop();
    printf("3\n");
    printf("network stop!\n");
}