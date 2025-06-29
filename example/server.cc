#include <bbt/network/TcpServer.hpp>
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/core/log/Logger.hpp>
#include <bbt/core/clock/Clock.hpp>

using namespace bbt::network;

int main()
{
    auto evthread = std::make_shared<EvThread>(std::make_shared<bbt::pollevent::EventLoop>());
    auto server = TcpServer::Create(evthread);


    server->Init();
    server->SetOnClose([](ConnId id){
        std::cout << bbt::core::clock::getnow_str() << "close connection " << id << std::endl;
    });

    server->SetOnTimeout([](ConnId id){
        std::cout << bbt::core::clock::getnow_str() << "timeout" << id << std::endl;
    });

    if (auto rlt = bbt::core::net::make_ip_address("127.0.0.1", 11001); rlt.IsErr())
    {
        std::cout << "make ip address failed! " << rlt.Err().CWhat() << std::endl;
        return -1;
    }
    else
    {
        auto err = server->AsyncListen(rlt.Ok(), [](ConnId id){
            std::cout << bbt::core::clock::getnow_str() << "connect new conn" << id << std::endl;
        });

        if (err.has_value())
            std::cout << err->CWhat() << std::endl;    
    }

    evthread->Start();

    evthread->Join();
    BBT_BASE_LOG_INFO("server stoped!");
}