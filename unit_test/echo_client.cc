#include <bbt/network/adapter/libevent/Network.hpp>
#include <bbt/core/log/Logger.hpp>

using namespace bbt::network;
using namespace bbt::network::libevent;
ConnCallbacks callbacks;

void InitCallbacks()
{

    callbacks.on_close_callback = 
    [](void* udata, const IPAddress& addr) {
        BBT_BASE_LOG_INFO("%s", addr.GetIPPort().c_str());
    };

    callbacks.on_err_callback =
    [](void* udata, const Errcode& err){
        BBT_BASE_LOG_ERROR("errno=%s", err.CWhat());
    };

    callbacks.on_recv_callback =
    [](libevent::ConnectionSPtr sptr, const char* data, size_t len){
        BBT_BASE_LOG_DEBUG("onrecv succ, size=%d", len);
    };

    callbacks.on_send_callback =
    [](libevent::ConnectionSPtr conn, ErrOpt err, size_t send_len){
        BBT_BASE_LOG_DEBUG("[%d] send succ=%d", conn->GetConnId(), send_len);
    };

    callbacks.on_timeout_callback =
    [](libevent::ConnectionSPtr conn){
        BBT_BASE_LOG_DEBUG("[%d] timeout", conn->GetConnId());
        conn->Close();
    };
}

int main(int args, char* argv[])
{
    if (args != 3) {
        printf("[usage] ./{exec_name} {ip} {port}\n");
        exit(-1);
    }
    char*   ip          = argv[1];
    int     port        = std::stoi(argv[2]);


    Network network;
    network.AutoInitThread(1);
    network.Start();
    ConnectionSPtr connection = nullptr;
    bbt::core::thread::CountDownLatch count_down_latch{1};
    const char* data = "hello world";
    InitCallbacks();

    auto err = network.AsyncConnect(ip, port, 1000,
    [&connection, &count_down_latch](auto err, interface::INetConnectionSPtr new_conn){
        if (err.has_value()) {
            BBT_BASE_LOG_ERROR("%s", err->CWhat());
            count_down_latch.Down();
            return;
        }
        auto sptr = std::static_pointer_cast<libevent::Connection>(new_conn);
        BBT_BASE_LOG_INFO("async connect succ! peeraddr=%s", sptr->GetPeerAddress().GetIPPort().c_str());
        sptr->SetOpt_Callbacks(callbacks);

        connection = sptr;
        count_down_latch.Down();
    });

    if (err.has_value())
        BBT_BASE_LOG_ERROR(err->CWhat());
    
    network.Start();
    count_down_latch.Wait();

    if (connection == nullptr)
        return -1;

    auto end_time = bbt::core::clock::nowAfter(bbt::core::clock::seconds(3));
    while (!bbt::core::clock::expired<bbt::core::clock::ms>(end_time)) {
        Assert(connection->AsyncSend(data, strlen(data)) >= 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}   