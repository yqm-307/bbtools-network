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
#include <interface/INetConnection.hpp>

namespace bbt::network
{

class NetConnection:
    public interface::INetConnection
{
public:
    NetConnection();
    ~NetConnection();

    virtual bool IsConnected() override;
    virtual bool IsClosed() override;
    virtual void Close() override;
    virtual void OnRecv(const char* data, size_t len) override;
    virtual void OnSend(size_t succ_len) override;
    virtual void OnClose() override;
    virtual void OnTimeout() override;

};

} // namespace bbt::network