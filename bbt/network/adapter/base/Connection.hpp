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
#include <bbt/base/templateutil/managerconn/ManagerBase.hpp>

#include <bbt/network/interface/INetConnection.hpp>
#include <bbt/base/net/IPAddress.hpp>

namespace bbt::network::libevent
{
    class Network;
}

namespace bbt::network::base
{

class ConnectionBase;
class NetworkBase;
typedef std::shared_ptr<ConnectionBase> BaseConnectionSPtr;
typedef std::unique_ptr<ConnectionBase> BaseConnectionUQPtr;
typedef std::weak_ptr<ConnectionBase>   BaseConnectionWKPtr;

/**
 * @brief 基础的Connection类
 * 
 */
class ConnectionBase:
    public bbt::network::interface::INetConnection
{
    friend class NetworkBase;
    friend class libevent::Network;
public:

    ConnectionBase(int socket, const bbt::net::IPAddress& addr);
    ~ConnectionBase();

    virtual bool            IsConnected() const override final;
    virtual bool            IsClosed() const override final;
    virtual void            Close() override;
    virtual void            OnRecv(const char* data, size_t len) override;
    virtual void            OnSend(const bbt::errcode::Errcode& err, size_t succ_len) override;
    virtual void            OnClose() override;
    virtual void            OnTimeout() override;
    virtual const bbt::net::IPAddress& GetPeerAddress() const override final;
    virtual evutil_socket_t GetSocket() const final;
    virtual ConnId          GetConnId() const override final;
protected:
    virtual void            CloseSocket() final; 
    virtual void            SetStatus(ConnStatus status) final;
    static ConnId           GenerateConnId();
private:
    int                     m_socket_fd{-1};
    bbt::net::IPAddress     m_peer_addr;
    volatile ConnStatus     m_conn_status{ConnStatus::DEFAULT};
    const ConnId            m_conn_id{0};
};

} // namespace bbt::network::base