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
#include <bbt/core/util/ManagerBase.hpp>

#include <bbt/network/Define.hpp>
#include <bbt/network/interface/INetwork.hpp>
#include <bbt/network/adapter/base/Connection.hpp>

namespace bbt::network::base
{

class NetworkBase:
    public bbt::network::interface::INetwork
{
public:
    NetworkBase();
    ~NetworkBase();

    virtual ErrTuple<interface::INetConnectionSPtr> Connect(const char* ip, short port) override;

    virtual ErrTuple<interface::INetConnectionSPtr> Accept(int listen_fd) override;

    virtual ErrOpt AsyncConnect(const char* ip, short port, int timeout_ms, const interface::IOnConnectCallback& onconnect_cb) override;

    virtual ErrOpt StartListen(const char* ip, short port, const interface::IOnAcceptCallback& onaccept_cb) override;

    virtual BaseConnectionSPtr GetConnById(ConnId conn_id) final;
protected:
    std::map<ConnId, BaseConnectionWKPtr> m_conns_map;  // 所有的连接
};

} // namespace bbt::network::base