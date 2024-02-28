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

ConnectionBase::ConnectionBase(int socket, const bbt::net::IPAddress& addr)
    :m_socket_fd(socket),
    m_peer_addr(addr)
{
    assert(m_socket_fd >= 0);

    m_conn_status = ConnStatus::CONNECTED;
}

ConnectionBase::~ConnectionBase()
{
    Close();

    if (m_socket_fd < 0)
        return;

    ::close(m_socket_fd);
    m_socket_fd = -1;
}

ConnId ConnectionBase::GetConnId() const
{
    return GetMemberId();
}

bool ConnectionBase::IsConnected() const
{
    return (m_conn_status == ConnStatus::CONNECTED);
}

bool ConnectionBase::IsClosed() const
{
    return (m_conn_status == ConnStatus::DECONNECTED);
}

const bbt::net::IPAddress& ConnectionBase::GetPeerAddress() const
{
    return m_peer_addr;
}

void ConnectionBase::Close()
{
    AssertWithInfo(false, "emply implementation!");
}

void ConnectionBase::CloseSocket()
{
    if (m_socket_fd < 0)
        return;

    ::close(m_socket_fd);
    m_socket_fd = -1;
}

void ConnectionBase::SetStatus(ConnStatus status)
{
    AssertWithInfo(status >= m_conn_status, "status can`t rollback!");  // 连接状态不允许回退
    m_conn_status = status;
}

void ConnectionBase::OnRecv(const char* data, size_t len)
{
    AssertWithInfo(false, "emply implementation!");
}

void ConnectionBase::OnSend(const Errcode& err, size_t succ_len)
{
    AssertWithInfo(false, "emply implementation!");
}

void ConnectionBase::OnClose()
{
    AssertWithInfo(false, "emply implementation!");
}

void ConnectionBase::OnTimeout()
{
    AssertWithInfo(false, "emply implementation!");
}

evutil_socket_t ConnectionBase::GetSocket() const
{
    return m_socket_fd;
}

}