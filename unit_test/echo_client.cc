#include <bbt/network/adapter/libevent/Network.hpp>
#include <bbt/base/Logger/Logger.hpp>

using namespace bbt::network;
using namespace bbt::network::libevent;
ConnCallbacks callbacks;

void InitCallbacks()
{

    callbacks.on_close_callback = 
    [](void* udata, const bbt::net::IPAddress& addr) {
        BBT_BASE_LOG_INFO("%s", addr.GetIPPort().c_str());
    };

    callbacks.on_err_callback =
    [](void* udata, const bbt::errcode::Errcode& err){
        BBT_BASE_LOG_ERROR("errno=%s", err.CWhat());
    };

    callbacks.on_recv_callback =
    [](libevent::ConnectionSPtr sptr, const char* data, size_t len){
        BBT_BASE_LOG_DEBUG("onrecv succ, size=%d", len);
    };

    callbacks.on_send_callback =
    [](libevent::ConnectionSPtr conn, bbt::errcode::ErrOpt err, size_t send_len){
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
    int console_debug_flag = 1;
    BBT_CONFIG_QUICK_SET_DYNAMIC_ENTRY(int, &console_debug_flag, bbt::config::BBT_LOG_STDOUT_OPEN);
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
    bbt::thread::CountDownLatch count_down_latch{1};
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

    auto end_time = bbt::timer::clock::nowAfter(bbt::timer::clock::seconds(3));
    while (!bbt::timer::clock::expired<bbt::timer::clock::ms>(end_time)) {
        Assert(connection->AsyncSend(data, strlen(data)) >= 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}   