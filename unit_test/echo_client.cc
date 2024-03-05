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
    [](void* udata, const Errcode& err){
        BBT_BASE_LOG_ERROR("%s", err.CWhat());
    };

    callbacks.on_recv_callback =
    [](libevent::ConnectionSPtr sptr, const char* data, size_t len){
        BBT_BASE_LOG_DEBUG("[%s]", data);
        sptr->AsyncSend(data, len);
    };

    callbacks.on_send_callback =
    [](libevent::ConnectionSPtr conn, const Errcode& err, size_t send_len){
        BBT_BASE_LOG_DEBUG("[%d] end succ=%d", conn->GetConnId(), send_len);
    };

    callbacks.on_timeout_callback =
    [](libevent::ConnectionSPtr conn){
        BBT_BASE_LOG_DEBUG("[%d] timeout", conn->GetConnId());
        conn->Close();
    };
}

int main(int args, char* argv[])
{
    if (args != 4) {
        printf("[usage] ./{exec_name} {ip} {port}");
        exit(-1);
    }
    int     thread_num  = 1;
    char*   ip          = argv[3];
    int     port        = std::stoi(argv[4]);

    Network network{thread_num};
    ConnectionSPtr connection = nullptr;
    bbt::thread::lock::CountDownLatch count_down_latch{1};
    const char* data = "hello world";

    network.AsyncConnect(ip, port,
    [&connection, &count_down_latch](auto err, interface::INetConnectionSPtr new_conn){
        auto sptr = std::static_pointer_cast<libevent::Connection>(new_conn);
        sptr->SetOpt_Callbacks(callbacks);

        connection = sptr;
        count_down_latch.down();
    });
    
    
    count_down_latch.wait();

    auto end_time = bbt::timer::clock::nowAfter(bbt::timer::clock::seconds(5));
    while (!bbt::timer::clock::expired<bbt::timer::clock::ms>(end_time)) {
        connection->AsyncSend(data, sizeof(data));
    }
}   