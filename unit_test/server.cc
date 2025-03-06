#include <bbt/network/adapter/libevent/Network.hpp>
#include <bbt/core/Logger/Logger.hpp>

typedef bbt::network::libevent::Network Network;
typedef bbt::network::libevent::ConnectionSPtr ConnectionSPtr;
typedef bbt::network::interface::INetConnectionSPtr INetConnectionSPtr;

using namespace bbt::network;

int main()
{
    Network network;
    network.AutoInitThread(1);
    std::vector<ConnectionSPtr> conn_vec;

    auto err = network.StartListen("127.0.0.1", 10010, [&network, &conn_vec](ErrOpt err, INetConnectionSPtr sptr){
        bbt::network::libevent::ConnCallbacks callbacks;
        std::shared_ptr<bbt::network::libevent::Connection> conn = std::dynamic_pointer_cast<bbt::network::libevent::Connection>(sptr);
        callbacks.on_err_callback = [](auto, const Errcode& err){
            BBT_BASE_LOG_ERROR("%s", err.CWhat());
        };
        callbacks.on_timeout_callback = [](ConnectionSPtr conn){
            conn->Close();
        };
        callbacks.on_close_callback = [](auto, const IPAddress& addr){
            BBT_BASE_LOG_INFO("close connection, %s", addr.GetIPPort().c_str());
        };

        conn->SetOpt_Callbacks(callbacks);
        conn->SetOpt_CloseTimeoutMS(500);
        conn_vec.push_back(conn);
        BBT_BASE_LOG_INFO("new connection!");
    });

    if (err.has_value()) {
        BBT_BASE_LOG_INFO("%s", err->CWhat());
        printf("%s\n", err->CWhat());
    }

    network.Start();
    while (1)
    {
        sleep(1);
    }

    BBT_BASE_LOG_INFO("server stoped!");
}