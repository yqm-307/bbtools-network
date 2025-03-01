/**
 * @file Network.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-23
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <bbt/network/adapter/base/Network.hpp>
#include <bbt/base/assert/Assert.hpp>
#include <bbt/base/uuid/EasyID.hpp>

namespace bbt::network::base
{

NetworkBase::NetworkBase()
{}

NetworkBase::~NetworkBase()
{}

bbt::errcode::ErrTuple<interface::INetConnectionSPtr> NetworkBase::Connect(const char* ip, short port)
{
    return {FASTERR_ERROR("empty implemention!"), nullptr};
}

bbt::errcode::ErrTuple<interface::INetConnectionSPtr> NetworkBase::Accept(int listen_fd)
{   
    return {FASTERR_ERROR("empty implemention!"), nullptr};
}

bbt::errcode::ErrOpt NetworkBase::AsyncConnect(const char* ip, short port, int timeout_ms, const interface::OnConnectCallback& onconnect_cb)
{
    return FASTERR_ERROR("empty implemention!");
}

bbt::errcode::ErrOpt NetworkBase::StartListen(const char* ip, short port, const interface::OnAcceptCallback& onaccept_cb)
{
    return FASTERR_ERROR("empty implemention!");
}


BaseConnectionSPtr NetworkBase::GetConnById(ConnId conn_id)
{
    auto conn_itor = m_conns_map.find(conn_id);
    if (conn_itor == m_conns_map.end())
        return nullptr;
    
    auto conn_sptr = conn_itor->second.lock();
    DebugAssertWithInfo(conn_sptr != nullptr, "this is a wrong!");
    return conn_sptr;
}


}