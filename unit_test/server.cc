#include <bbt/network/adapter/libevent/Network.hpp>

typedef bbt::network::libevent::Network Network;
typedef bbt::network::libevent::ConnectionSPtr ConnectionSPtr;
typedef bbt::network::Errcode Errcode;
typedef bbt::network::interface::INetConnectionSPtr INetConnectionSPtr;

int main()
{
    Network network{1, "127.0.0.1", 10010};

    auto err = network.StartListen([&network](const Errcode& err, ConnectionSPtr sptr){
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