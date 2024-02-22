/**
 * @file INetwork.hpp
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
#include <bbt/network/interface/INetConnection.hpp>

namespace bbt::network::interface
{

typedef std::function<void(Errcode, INetConnection*)> OnConnectCallback;
typedef std::function<void(Errcode, INetConnection*)> OnAcceptCallback;


class INetwork
{
public:
    /**
     * @brief 主动建立与地址{ip:port}的主机的连接，在连接结果完成前阻塞
     * 连接可能因为成功返回，也可能因为失败返回
     * 
     * @param ip            对端ip
     * @param port          对端端口
     * @return std::pair<Errcode, INetConnection*> 
     */
    virtual std::pair<Errcode, INetConnection*> Connect(const char* ip, short port) = 0;

    /**
     * @brief 从listen_fd获取一个新的来自其他主机的连接，在获取到结果前
     * 阻塞；连接可能因为成功返回，也可能因为失败返回
     * 
     * @param listen_fd     监听套接字
     * @return std::pair<Errcode, INetConnection*> 
     */
    virtual std::pair<Errcode, INetConnection*> Accept(int listen_fd) = 0;

    /**
     * @brief 与地址{ip:port}的主机建立连接，当连接完成时通过onconnect_cb
     * 通知调用者；此函数调用后立即返回；连接可能成功也可能失败
     * 
     * @param ip            对端ip
     * @param port          对端端口
     * @param onconnect_cb  连接结果回调
     * @return Errcode 
     */
    virtual Errcode AsyncConnect(const char* ip, short port, const OnConnectCallback& onconnect_cb) = 0;

    /**
     * @brief 接收一个连接，结果通过onaccept_cb通知调用者；此函数调用后
     * 立即返回；连接可能成功也可能失败
     * 
     * @param listen_fd     监听地址
     * @param onaccept_cb   连接结果回调
     * @return Errcode 
     */
    virtual Errcode AsyncAccept(int listen_fd, const OnAcceptCallback& onaccept_cb) = 0;
};

}