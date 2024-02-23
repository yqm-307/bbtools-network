/**
 * @file INetConenct.hpp
 * @author yangqingmiao
 * @brief 网络连接接口类定义
 * @version 0.1
 * @date 2024-02-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <memory>
#include <bbt/network/interface/ICallback.hpp>
#include <bbt/base/net/IPAddress.hpp>

namespace bbt::network::interface
{

class INetConnection;

typedef std::shared_ptr<INetConnection> INetConnectionSPtr;
typedef std::unique_ptr<INetConnection> INetConnectionUQPtr;


class INetConnection:
    public INetCallback
{
public:
    /**
     * @brief 获取连接对端的ip地址
     * 
     * @return const bbt::net::IPAddress& 
     */
    virtual const bbt::net::IPAddress& GetPeerAddress() const = 0;

    /**
     * @brief 获取当前连接对象的连接id
     * 
     * @return ConnId
     */
    virtual ConnId GetConnId() const = 0;
};

}
