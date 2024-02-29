#include <bbt/network/adapter/libevent/Network.hpp>

typedef bbt::network::libevent::Network Network;
typedef bbt::network::libevent::Connection Connection;
typedef bbt::network::libevent::ConnectionSPtr ConnectionSPtr;
typedef bbt::network::Errcode Errcode;
typedef bbt::network::interface::INetConnectionSPtr INetConnectionSPtr;


int main()
{
    Network network{1, "127.0.0.1", 10011};

    network.AsyncConnect("127.0.0.1", 10010,
    [&network](Errcode err, INetConnectionSPtr i_sptr){
        auto ptr = std::static_pointer_cast<Connection>(i_sptr);
        if (!err) {
            printf("connect err! %s\n", err.CWhat());
        }

        printf("connect success!\n");
    });

    network.Start();
    // 防止主线程退出
    for (int i = 0; i < 2; ++i)
        sleep(1);
    network.Stop();
    printf("network stop!\n");
}