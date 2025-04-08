#include <event2/thread.h>
#include <bbt/network/TcpClient.hpp>
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/core/clock/Clock.hpp>

using namespace bbt::core::errcode;
using namespace bbt::network;

int main()
{
    auto evthread = std::make_shared<EvThread>(std::make_shared<bbt::pollevent::EventLoop>());
    auto client = std::make_shared<TcpClient>(evthread);

    client->Init();

    // 设置连接1s超时自动关闭
    client->SetConnectionTimeout(1000);

    client->SetOnClose([](ConnId id){
        std::cout << bbt::core::clock::getnow_str() << "close connection " << id << std::endl;
    });

    client->SetOnConnect([](auto id, ErrOpt err){
        std::cout << bbt::core::clock::getnow_str() << "onconnect! " << (err.has_value() ? err->CWhat() : "succ") << std::endl;
    });

    client->AsyncConnect({"127.0.0.1", 11001}, 100);

    evthread->Start();

    while(1)
    {
        sleep(1);
    }
}