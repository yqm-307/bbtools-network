#pragma once
#include <bbt/core/buffer/Buffer.hpp>
#include <bbt/network/Define.hpp>
#include <bbt/network/interface/INetConnection.hpp>


namespace bbt::network::interface
{

class ITcpClient
{
public:
    ITcpClient() = default;
    virtual ~ITcpClient() = default;

    /**
     * @brief 主动建立与地址{ip:port}的主机的连接，连接成功后，会调用onconnect_cb
     * 
     * @param addr          服务器监听地址
     * @param connect_timeout  连接超时时间，单位ms
     * @param onconnect_cb  连接结果回调
     * @return ErrOpt
     */
    virtual ErrOpt  AsyncConnect(const IPAddress& addr, int connect_timeout, const IOnConnectCallback& onconnect_cb) = 0;
    virtual bool    IsConnecting() = 0;
    virtual ErrOpt  Send(const bbt::core::Buffer& buffer) = 0;
    virtual void    OnSend(ErrOpt err, size_t send_len) = 0;
    virtual ErrOpt  OnRecv(bbt::core::Buffer& buffer) = 0;
    virtual ErrOpt  Close() = 0;
    virtual ErrOpt  OnClose() = 0;
    virtual void    OnTimeout() = 0;
    virtual void    SetTimeout(int connection_timeout) = 0;

};

} // namespace bbt::network::interface