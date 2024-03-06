/**
 * @file Network.hpp
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

#include <bbt/network/Define.hpp>
#include <bbt/network/interface/INetwork.hpp>
#include <bbt/network/adapter/base/Connection.hpp>

namespace bbt::network::base
{

class NetworkBase:
    public bbt::network::interface::INetwork,
    public bbt::templateutil::ManagerBase<ConnId, ConnectionBase>
{
public:
    NetworkBase();
    ~NetworkBase();

    virtual std::pair<Errcode, interface::INetConnectionSPtr> Connect(const char* ip, short port) override;

    virtual std::pair<Errcode, interface::INetConnectionSPtr> Accept(int listen_fd) override;

    virtual Errcode AsyncConnect(const char* ip, short port, int timeout_ms, const interface::OnConnectCallback& onconnect_cb) override;

    virtual Errcode AsyncAccept(int listen_fd, const interface::OnAcceptCallback& onaccept_cb) override;

    virtual BaseConnectionSPtr GetConnById(ConnId conn_id) final;

    virtual bool        OnMemberCreate(MemberPtr member) final;

    virtual bool        OnMemberDestory(KeyType member) final;

    virtual KeyType     GenerateKey(MemberPtr) final;
protected:
    std::map<ConnId, BaseConnectionWKPtr> m_conns_map;  // 所有的连接
};

} // namespace bbt::network::base