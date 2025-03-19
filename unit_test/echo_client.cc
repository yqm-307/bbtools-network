#include <bbt/network/TcpClient.hpp>
#include <bbt/core/log/Logger.hpp>
#include <bbt/network/EvThread.hpp>
#include <bbt/pollevent/EventLoop.hpp>
#include <bbt/pollevent/Event.hpp>

using namespace bbt::network;
std::shared_ptr<Event> Timer = nullptr;

std::shared_ptr<TcpClient> NewClient(std::shared_ptr<EvThread> evthread)
{
    auto client = std::make_shared<TcpClient>(evthread);

    // ctrl z 事件监听
    evthread->RegisterEvent(0, EventOpt::SIGNAL, [](auto, short events, auto){
        std::cout << "[Echo Client] ctrl z" << std::endl;
    });

    client->Init();
    client->SetOnConnect([client, evthread](auto id, auto err){
        if (err.has_value()) {
            std::cout << "[Echo Client] connect error: " << err->CWhat() << std::endl;
            return;
        }
        else
            std::cout << "[Echo Client] connect success! " << id << std::endl;

        Timer = evthread->RegisterEvent(0, EventOpt::TIMEOUT | EventOpt::PERSIST, [id, client](auto, short events, auto){
            client->Send(bbt::core::Buffer{"hello world!"});
        });

        Assert(Timer->StartListen(10) == 0);
    });

    client->SetOnClose([client](auto id){
        std::cout << "[Echo Client] close success! " << id << std::endl;
    });

    client->SetOnRecv([client](auto id, auto& buffer){
        std::cout << "[Echo Client] recv: " << buffer.Peek() << std::endl;
    });

    client->SetOnSend([client](auto id, auto err, auto send_len){
        if (err.has_value())
            std::cout << "[Echo Client] send error: " << err->CWhat() << std::endl;
        else
            std::cout << "[Echo Client] send success: " << send_len << std::endl;
    });

    return client;
}

int main(int args, char* argv[])
{
    if (args != 4) {
        printf("[usage] ./{exec_name} {ip} {port} {n_client}\n");
        exit(-1);
    }
    char*   ip          = argv[1];
    int     port        = std::stoi(argv[2]);
    int     max_client  = std::stoi(argv[3]);

    std::vector<std::shared_ptr<TcpClient>> clients;
    auto evthread = std::make_shared<EvThread>(std::make_shared<bbt::pollevent::EventLoop>());

    for (int i = 0; i < max_client; ++i) {
        auto client = NewClient(evthread);
        if (auto err = client->AsyncConnect(bbt::core::net::IPAddress{ip, port}, 1000); err.has_value()) {
            std::cout << "[Echo Client] AsyncConnect error: " << err->CWhat() << std::endl;
            continue;
        }
        clients.push_back(client);
    }

    evthread->Start();
    evthread->Join();
}   