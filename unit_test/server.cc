#include <bbt/network/TcpServer.hpp>
#include <bbt/network/EvThread.hpp>
#include <bbt/core/log/Logger.hpp>

using namespace bbt::network;

int main()
{
    auto evthread = std::make_shared<EvThread>(std::make_shared<bbt::pollevent::EventLoop>());
    auto server = std::make_shared<TcpServer>(evthread);


    server->Init();
    server->SetOnClose([](ConnId id){
        std::cout << "close connection " << id << std::endl;
    });

    server->SetOnTimeout([](ConnId id){
        std::cout << "timeout" << id << std::endl;
    });

    auto err = server->AsyncListen({"", 11001}, [](ConnId id){
        std::cout << "connect new conn" << id << std::endl;
    });

    if (err.has_value())
        std::cout << err->CWhat() << std::endl;    

    evthread->Start();

    while (1)
    {
        sleep(1);
    }

    BBT_BASE_LOG_INFO("server stoped!");
}