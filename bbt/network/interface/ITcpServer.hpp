#pragma once
#include <bbt/network/Define.hpp>
#include <bbt/core/buffer/Buffer.hpp>
#include <bbt/network/interface/INetConnection.hpp>

namespace bbt::network::interface
{

class ITcpServer
{
public:
    ITcpServer() = default;
    virtual ~ITcpServer() = default;

    virtual bbt::core::errcode::ErrOpt AsyncListen(const bbt::core::net::IPAddress& addr) = 0;
    virtual void OnAccept(ErrOpt err, INetConnectionSPtr conn) = 0;
    virtual bbt::core::errcode::ErrOpt StopListen(const bbt::core::net::IPAddress& addr) = 0;
    virtual const bbt::core::net::IPAddress& GetListenAddress() const = 0;
    virtual bool IsListening() = 0;
    virtual void OnTimeout(ConnId connid) = 0;
    virtual void OnClose(ConnId connid) = 0;
    virtual void SetTimeout(int connection_timeout) = 0;
    virtual void OnSend(ConnId connid, ErrOpt err, size_t send_len) = 0;
    virtual void OnRecv(ConnId connid, bbt::core::Buffer& buffer) = 0;
};

};