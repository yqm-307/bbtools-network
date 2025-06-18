#include <bbt/network/TcpClient.hpp>
#include <bbt/core/log/Logger.hpp>
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/pollevent/EventLoop.hpp>
#include <bbt/pollevent/Event.hpp>
#include <bbt/core/clock/Clock.hpp>

using namespace bbt::network;
using namespace bbt::core::clock;
std::shared_ptr<Event> Print = nullptr;

std::map<ConnId, std::shared_ptr<Event>> SendEventMap;

std::map<ConnId, std::tuple<size_t, size_t>> MonitorInfoMap; // connid -> (recv, send)
std::mutex MonitorInfoMapMutex;

std::shared_ptr<TcpClient> NewClient(std::shared_ptr<EvThread> evthread)
{
    auto client = TcpClient::Create(evthread);

    // ctrl z 事件监听
    evthread->RegisterEvent(0, EventOpt::SIGNAL, [](auto, short events, auto){
        std::cout << getnow_str() << "[Echo Client] ctrl z" << std::endl;
    });

    client->Init();
    client->SetOnConnect([client, evthread](auto id, auto err){
        if (err.has_value()) {
            std::cout << getnow_str() << "[Echo Client] connect error: " << err->CWhat() << std::endl;
            return;
        }
        else
            std::cout << getnow_str() << "[Echo Client] connect success! " << id << std::endl;

        std::lock_guard<std::mutex> _(MonitorInfoMapMutex);

        SendEventMap[id] = evthread->RegisterEvent(0, EventOpt::TIMEOUT | EventOpt::PERSIST, [id, client](auto, short events, auto){
            client->Send(bbt::core::Buffer{"hello world!"});
        });

        Print = evthread->RegisterEvent(0, EventOpt::TIMEOUT | EventOpt::PERSIST, [id, client](auto, short events, auto){
            std::lock_guard<std::mutex> _(MonitorInfoMapMutex);

            for (auto& [connid, event] : SendEventMap) {
                auto it = MonitorInfoMap.find(connid);
                if (it != MonitorInfoMap.end()) {
                    auto& [recv, send] = it->second;
                    std::cout << "[EchoClient] connid: " << connid
                        << ", recv: " << recv
                        << ", send: " << send
                        << std::endl;
                }
            }
        });

        Assert(Print->StartListen(1000) == 0);
        Assert(SendEventMap[id]->StartListen(10) == 0);

        MonitorInfoMap[id] = std::make_tuple(0, 0);
    });

    client->SetOnClose([client](ConnId id){
        std::cout << getnow_str() << "[Echo Client] close success! " << id << " err=" << errno << std::endl;
    });

    client->SetOnRecv([client](ConnId id, auto& buffer){
        // std::cout << "[Echo Client] recv: " << buffer.Peek() << std::endl;
        std::lock_guard<std::mutex> _(MonitorInfoMapMutex);
        auto it = MonitorInfoMap.find(id);
        if (it != MonitorInfoMap.end()) {
            auto& [recv, send] = it->second;
            recv += buffer.Size();
        }
    });

    client->SetOnSend([client](ConnId id, auto err, auto send_len){
        if (err.has_value())
            std::cout << getnow_str() << "[Echo Client] send error: " << err->CWhat() << std::endl;
        else {
            std::lock_guard<std::mutex> _(MonitorInfoMapMutex);
            auto it = MonitorInfoMap.find(id);
            if (it != MonitorInfoMap.end()) {
                auto& [recv, send] = it->second;
                send += send_len;
            }
        }
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
        // if (auto err = client->AsyncConnect(bbt::core::net::IPAddress{ip, port}, 3000); err.has_value()) {
        //     std::cout << getnow_str() << "[Echo Client] AsyncConnect error: " << err->CWhat() << std::endl;
        //     continue;
        // }
        if (auto err = client->Connect(bbt::core::net::IPAddress{ip, port}, 3000); err.has_value()) {
            std::cout << getnow_str() << "[Echo Client] Connect error: " << err->CWhat() << std::endl;
            continue;
        }
        clients.push_back(client);
    }

    evthread->Start();
    evthread->Join();
}   