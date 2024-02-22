/**
 * @file Connection.hpp
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <bbt/network/interface/INetConnection.hpp>
#include <bbt/network/Define.hpp>
#include <bbt/base/net/IPAddress.hpp>


namespace bbt::network::base
{


/**
 * @brief 基础的Connection类
 * 
 */
class BaseConnection:
    public bbt::network::interface::INetConnection
{
public:
    BaseConnection(int socket, const bbt::net::IPAddress& addr);
    ~BaseConnection();

    virtual bool            IsConnected() const override final;
    virtual bool            IsClosed() const override final;
    virtual void            Close() override;
    virtual void            OnRecv(const char* data, size_t len) override;
    virtual void            OnSend(size_t succ_len) override;
    virtual void            OnClose() override;
    virtual void            OnTimeout() override;
    virtual const bbt::net::IPAddress& GetPeerAddress() const override final;
private:
    int                     m_socket_fd{-1};
    bbt::net::IPAddress     m_peer_addr;
    ConnStatus              m_conn_status{ConnStatus::DEFAULT};

};

} // namespace bbt::network::base