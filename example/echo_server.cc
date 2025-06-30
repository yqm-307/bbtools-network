#include <bbt/network/TcpServer.hpp>
#include <bbt/core/log/Logger.hpp>
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/core/clock/Clock.hpp>

using namespace bbt::network;
using namespace bbt::core::clock;

class EchoServer
{
    struct UData{ConnId connid{0};};
public:
    EchoServer(const bbt::core::net::IPAddress& addr, std::shared_ptr<EvThread> evthread):
        m_server(TcpServer::Create(evthread)),
        m_addr(addr)
    {
        m_server->Init();
        m_server->SetTimeout(5000);
        m_server->SetOnTimeout([this](auto connid){
            std::cout << getnow_str() << "[EchoServer] on timeout " << connid << std::endl;
        });
        m_server->SetOnClose([this](auto connid){
            std::cout << getnow_str() << "[EchoServer] on close " << connid << std::endl;
        });
        m_server->SetOnRecv([this](auto connid, const bbt::core::Buffer& buffer){
            // std::cout << getnow_str() << "[EchoServer] on recv " << connid << " data: " << buffer.Peek() << std::endl;
            auto err = m_server->Send(connid, buffer);
            if (err.has_value())
                std::cout << getnow_str() << "[EchoServer] send error: " << err->CWhat() << std::endl;
        });
        m_server->SetOnSend([this](ConnId id, bbt::core::errcode::ErrOpt err, size_t len){});
        m_server->SetOnErr([this](auto connid, const bbt::core::errcode::Errcode& err){
            std::cout << getnow_str() << "[EchoServer] error: " << err.CWhat() << std::endl;
            m_server->Close(connid);
        });
    }

    ~EchoServer()
    {
    }

    void Start()
    {
        auto err = m_server->AsyncListen(m_addr, [](bbt::network::ConnId connid){
            std::cout << getnow_str() << "[EchoServer] onaccept " << connid << std::endl;
        });

        if (err.has_value())
            std::cout << getnow_str() << "[EchoServer] listen error: " << err->CWhat() << std::endl;
    }
private:
    
    std::shared_ptr<TcpServer> m_server{nullptr};
    bbt::core::net::IPAddress m_addr;
};

int main(int args, char* argv[])
{
    if (args != 2) {
        printf("[usage] ./{exec_name} {port}\n");
        exit(-1);
    }

    std::shared_ptr<EvThread> evthread = std::make_shared<EvThread>(std::make_shared<bbt::pollevent::EventLoop>());

    auto rlt = bbt::core::net::make_ip_address("127.0.0.1", std::atoi(argv[1]));
    if (rlt.IsErr()) {
        std::cout << "make ip address failed! " << rlt.Err().CWhat() << std::endl;
        return -1;
    }

    EchoServer server{rlt.Ok(), evthread};

    server.Start();

    evthread->Start();
    evthread->Join();
}   