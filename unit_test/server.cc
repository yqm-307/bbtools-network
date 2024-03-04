#include <bbt/network/adapter/libevent/Network.hpp>

typedef bbt::network::libevent::Network Network;
typedef bbt::network::libevent::ConnectionSPtr ConnectionSPtr;
typedef bbt::network::Errcode Errcode;
typedef bbt::network::interface::INetConnectionSPtr INetConnectionSPtr;

int main()
{
    Assert(bbt::network::GlobalInit());
    Network network{1, "127.0.0.1", 10010};
    std::vector<ConnectionSPtr> conn_vec;

    auto err = network.StartListen([&network, &conn_vec](const Errcode& err, ConnectionSPtr sptr){
        bbt::network::libevent::ConnCallbacks callbacks;
        callbacks.on_err_callback = [](auto, const bbt::network::Errcode& err){
            printf("[onerr] %s\n", err.CWhat());
        };
        callbacks.on_timeout_callback = [](ConnectionSPtr conn){
            conn->Close();
        };

        sptr->SetOpt_Callbacks(callbacks);
        sptr->SetOpt_CloseTimeoutMS(500);
        conn_vec.push_back(sptr);
        printf("new connection! err=[%s]\n", err.CWhat());
    });

    if (!err) {
        printf("%s\n", err.CWhat());
    }

    network.Start();
    while (1)
    {
        sleep(1);
    }

    printf("server stoped!\n");
}