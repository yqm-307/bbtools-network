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
#include <bbt/network/Define.hpp>
#include <bbt/network/adapter/base/Connection.hpp>
#include <bbt/network/adapter/libevent/Event.hpp>

namespace bbt::network::libevent
{

class Connection:
    public base::BaseConnection
{
public:
    Connection(EventBase* base, evutil_socket_t socket, bbt::net::IPAddress& ipaddr);
    ~Connection();

    virtual void Init(evutil_socket_t fd);
    virtual void Close() override;
    virtual void OnRecv(const char* data, size_t len) override;
    virtual void OnSend(size_t succ_len) override;
    virtual void OnClose() override;
    virtual void OnTimeout() override;

};

}