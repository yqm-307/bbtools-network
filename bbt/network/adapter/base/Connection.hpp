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
#include <bbt/core/util/ManagerBase.hpp>
#include <bbt/network/interface/INetConnection.hpp>

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

    ConnectionBase(int socket, const IPAddress& addr);
    ~ConnectionBase();

    virtual bool            IsConnected() const override final;
    virtual bool            IsClosed() const override final;
    virtual void            Close() override;
    virtual void            OnRecv(const char* data, size_t len) override;
    virtual void            OnSend(ErrOpt err, size_t succ_len) override;
    virtual void            OnClose() override;
    virtual void            OnTimeout() override;
    virtual const IPAddress& GetPeerAddress() const override final;
    virtual evutil_socket_t GetSocket() const final;
    virtual ConnId          GetConnId() const override final;
protected:
    virtual void            CloseSocket() final; 
    virtual void            SetStatus(ConnStatus status) final;
    static ConnId           GenerateConnId();
private:
    int                     m_socket_fd{-1};
    IPAddress               m_peer_addr;
    volatile ConnStatus     m_conn_status{ConnStatus::emCONN_DEFAULT};
    const ConnId            m_conn_id{0};
};

} // namespace bbt::network::base