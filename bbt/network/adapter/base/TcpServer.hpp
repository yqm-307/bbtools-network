#pragma once
#include <bbt/network/adapter/base/Connection.hpp>
#include <bbt/network/interface/ITcpServer.hpp>

namespace bbt::network::base
{

class TcpServer:
    public interface::ITcpServer
{
public:
    TcpServer();
    virtual ~TcpServer();



private:
    std::unordered_map<ConnId, BaseConnectionSPtr> m_conns_map;
    std::mutex m_conns_map_mutex;
    bbt::core::net::IPAddress m_listen_addr;
};

} // namespace bbt::network::base