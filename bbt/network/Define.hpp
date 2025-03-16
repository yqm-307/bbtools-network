/**
 * @file Define.hpp
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <cassert>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <functional>
#include <map>
#include <vector>
#include <memory>

#include <event2/event.h>
#include <event2/thread.h>

#include <bbt/core/util/Assert.hpp>
#include <bbt/core/Attribute.hpp>
#include <bbt/core/net/IPAddress.hpp>
#include <bbt/core/buffer/Buffer.hpp>
#include <bbt/network/Errcode.hpp>
#include <bbt/pollevent/EventLoop.hpp>

namespace bbt::network
{

using namespace bbt::core::errcode;
using namespace bbt::core::net;
using namespace bbt::pollevent;

#define BBT_NETWORK_MODULE "[bbt::network]"

// 空闲断开连接时间
#define CONNECTION_FREE_TIMEOUT_MS 5000
// 发送超时失败
#define SEND_DATA_TIMEOUT_MS 2000
// 连接超时
#define CONNECT_TIMEOUT_MS 2000

// 连接状态枚举
enum ConnStatus
{
    emCONN_DEFAULT     = 0,    // 默认状态
    emCONN_CONNECTED   = 1,    // 连接完成
    emCONN_DECONNECTED = 2,    // 断开连接
};

// 网络状态
enum NetworkStatus
{
    emNETWORK_DEFAULT     = 0,
    emNETWORK_STARTING    = 1,
    emNETWORK_RUNNING     = 2,
    emNETWORK_STOP        = 3,
};

/* 线程状态枚举 */
enum IOThreadRunStatus
{
    Default = 0,
    Running = 1,
    Finish = 2,
};

class TcpServer;
class TcpClient;
class EvThread;

// 连接id
typedef uint64_t ConnId;
// 事件id
typedef uint64_t EventId;
typedef int IOThreadID;

namespace detail
{
class Connection;

typedef std::shared_ptr<Connection> ConnectionSPtr;
typedef std::function<void(ConnectionSPtr, const char*, size_t)>  OnRecvCallback;
typedef std::function<void(ConnectionSPtr, ErrOpt, size_t)>   OnSendCallback;
typedef std::function<void(ConnId, const IPAddress& )>  OnCloseCallback;
typedef std::function<void(ConnectionSPtr)>             OnTimeoutCallback;
typedef std::function<void(const Errcode&)>             OnConnErrorCallback;

struct ConnCallbacks
{
    OnRecvCallback      on_recv_callback{nullptr};
    OnSendCallback      on_send_callback{nullptr};
    OnCloseCallback     on_close_callback{nullptr};
    OnTimeoutCallback   on_timeout_callback{nullptr};
    OnConnErrorCallback     on_err_callback{nullptr};
};

} // namespace detail

typedef std::function<void(ConnId)> OnTimeoutFunc;
typedef std::function<void(ConnId)> OnCloseFunc;
typedef std::function<void(ConnId, ErrOpt, size_t)> OnSendFunc; 
typedef std::function<void(ConnId, const bbt::core::Buffer&)> OnRecvFunc;
typedef std::function<void(const Errcode&)> OnErrFunc;
typedef std::function<void(ConnId)> OnAcceptFunc;
typedef std::function<void(ErrOpt)> OnConnectFunc;

} // namespace bbt::network

#define FASTERR(info, type) std::make_optional<Errcode>(info, type)
#define FASTERR_ERROR(info) FASTERR(BBT_NETWORK_MODULE info, bbt::network::ErrType::ERRTYPE_ERROR)
#define FASTERR_NOTHING std::nullopt