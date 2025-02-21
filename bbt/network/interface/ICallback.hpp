/**
 * @file ICallback.hpp
 * @author yangqingmiao
 * @brief 网络连接的回调定义
 * @version 0.1
 * @date 2024-02-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <stdio.h>
#include <bbt/network/Define.hpp>

namespace bbt::network::interface
{

/**
 * @brief 网络连接事件回调
 * 
 * 接口：
 * 1、IsConnected
 * 2、IsClosed
 * 3、Close
 */
class ICallback
{
public:
    /**
     * @brief 连接是否已经建立
     * 
     * @return true 
     * @return false 
     */
    virtual bool IsConnected() const = 0;

    /**
     * @brief 连接是否已经关闭
     * 
     * @return true 
     * @return false 
     */
    virtual bool IsClosed() const = 0;

    /**
     * @brief 主动关闭此连接 
     * 
     */
    virtual void Close() = 0;

    virtual void OnError(const bbt::errcode::Errcode& error) = 0;
};



/**
 * @brief 网络IO相关回调接口
 * 
 * 接口:
 * 1、IsConnected
 * 2、IsClosed
 * 3、Close
 * 4、OnRecv
 * 5、OnSend
 * 6、OnClose
 * 7、OnTimeout
 */
class INetCallback:
    public ICallback
{
public:
    // virtual bool IsConnected() = 0;
    // virtual bool IsClosed() = 0;
    // virtual void Close() = 0;

    /**
     * @brief 从网络获取数据传入
     * 
     * @param data 
     * @param len 
     */
    virtual void OnRecv(const char* data, size_t len) = 0;

    /**
     * @brief 向网络发送数据 
     * 
     * @param succ_len 成功发送的字节数 
     */
    virtual void OnSend(const bbt::errcode::Errcode& err, size_t succ_len) = 0;

    /**
     * @brief 连接关闭事件
     */
    virtual void OnClose() = 0;

    /**
     * @brief 连接超时事件
     */
    virtual void OnTimeout() = 0;

};



class IDBCallback:
    public ICallback
{
};

}