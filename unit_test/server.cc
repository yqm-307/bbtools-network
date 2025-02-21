#include <bbt/network/adapter/libevent/Network.hpp>
#include <bbt/base/Logger/Logger.hpp>

typedef bbt::network::libevent::Network Network;
typedef bbt::network::libevent::ConnectionSPtr ConnectionSPtr;
typedef bbt::network::Errcode Errcode;
typedef bbt::network::interface::INetConnectionSPtr INetConnectionSPtr;

int main()
{
    int console_debug_flag = 1;
    BBT_CONFIG_QUICK_SET_DYNAMIC_ENTRY(int, &console_debug_flag, bbt::config::BBT_LOG_STDOUT_OPEN);
    Network network;
    network.AutoInitThread(1);
    std::vector<ConnectionSPtr> conn_vec;

    auto err = network.StartListen("127.0.0.1", 10010, [&network, &conn_vec](const Errcode& err, ConnectionSPtr sptr){
        bbt::network::libevent::ConnCallbacks callbacks;
        callbacks.on_err_callback = [](auto, const bbt::errcode::Errcode& err){
            BBT_BASE_LOG_ERROR("%s", err.CWhat());
        };
        callbacks.on_timeout_callback = [](ConnectionSPtr conn){
            conn->Close();
        };
        callbacks.on_close_callback = [](auto, const bbt::net::IPAddress& addr){
            BBT_BASE_LOG_INFO("close connection, %s", addr.GetIPPort().c_str());
        };

        sptr->SetOpt_Callbacks(callbacks);
        sptr->SetOpt_CloseTimeoutMS(500);
        conn_vec.push_back(sptr);
        BBT_BASE_LOG_INFO("new connection!");
    });

    if (err.IsErr()) {
        BBT_BASE_LOG_INFO("%s", err.CWhat());
        printf("%s\n", err.CWhat());
    }

    network.Start();
    while (1)
    {
        sleep(1);
    }

    BBT_BASE_LOG_INFO("server stoped!");
}