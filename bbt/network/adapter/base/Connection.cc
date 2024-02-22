/**
 * @file Connection.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <bbt/network/adapter/base/Connection.hpp>

namespace bbt::network::base
{

BaseConnection::BaseConnection(int socket, const bbt::net::IPAddress& addr)
    :m_socket_fd(socket),
    m_peer_addr(addr)
{
    assert(m_socket_fd >= 0);
}

BaseConnection::~BaseConnection()
{
    Close();

    ::close(m_socket_fd);
    m_socket_fd = -1;


}

bool BaseConnection::IsConnected()
{
    
}

// virtual bool BaseConnection::IsClosed() override;
// virtual void BaseConnection::Close() override;
// virtual void BaseConnection::OnRecv(const char* data, size_t len) override;
// virtual void BaseConnection::OnSend(size_t succ_len) override;
// virtual void BaseConnection::OnClose() override;
// virtual void BaseConnection::OnTimeout() override;

}