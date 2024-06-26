#include <bbt/network/adapter/libevent/Network.hpp>
#include <event2/thread.h>

typedef bbt::network::libevent::Network Network;
typedef bbt::network::libevent::Connection Connection;
typedef bbt::network::libevent::ConnectionSPtr ConnectionSPtr;
typedef bbt::network::Errcode Errcode;
typedef bbt::network::interface::INetConnectionSPtr INetConnectionSPtr;


int main()
{
    Assert(bbt::network::GlobalInit());

    Network network;
    network.AutoInitThread(1);
    std::vector<ConnectionSPtr> conn_vec;

    network.AsyncConnect("127.0.0.1", 10010, 1000,
    [&network, &conn_vec](Errcode err, INetConnectionSPtr i_sptr){
        if (err.IsErr()) {
            printf("connect err! %s\n", err.CWhat());
            return;
        }
        auto ptr = std::static_pointer_cast<Connection>(i_sptr);
        bbt::network::libevent::ConnCallbacks callback;
        callback.on_err_callback = [](auto, const Errcode& err){
            printf("[onerr] %s\n", err.CWhat());
        };
        callback.on_timeout_callback = [](auto conn){
            conn->Close();
        };

        ptr->SetOpt_Callbacks(callback);
        conn_vec.push_back(ptr);
        printf("connect success!\n");
    });

    network.Start();
    // 防止主线程退出
    for (int i = 0; i < 2; ++i)
        sleep(5);
    network.Stop();
    printf("network stop!\n");
}