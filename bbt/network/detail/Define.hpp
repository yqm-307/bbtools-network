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
#include <bbt/core/errcode/Errcode.hpp>
#include <bbt/pollevent/EventLoop.hpp>

namespace bbt::network
{

// using namespace bbt::core::errcode;
using namespace bbt::core::net;
using namespace bbt::pollevent;

#define BBT_NETWORK_MODULE "[bbt::network]"

// 空闲断开连接时间
#define CONNECTION_FREE_TIMEOUT_MS 5000
// 发送超时失败
#define SEND_DATA_TIMEOUT_MS 2000
// 连接超时
#define CONNECT_TIMEOUT_MS 2000

enum emErr : bbt::core::errcode::ErrType
{
    ERRTYPE_NOTHING = 0,    // 没有问题
    ERRTYPE_ERROR = 1,      // 错误

    ERRTPYE_EVENTLOOP_LOOP_ERR_EXIT             = 100,          // 因为错误退出循环
    ERRTYPE_EVENTLOOP_LOOP_EXIT                 = 101,          // 退出循环

    ERRTYPE_NETWORK_RECV_TRY_AGAIN              = 201,          // 未准备好，请尝试
    ERRTYPE_NETWORK_RECV_CONNREFUSED            = 202,          // 连接被服务器拒绝
    ERRTYPE_NETWORK_RECV_EOF                    = 203,          // 连接关闭
    ERRTYPE_NETWORK_RECV_OTHER_ERR              = 204,          // 其他错误

    ERRTYPE_SEND_TIMEOUT                        = 301,          // 发送超时

    ERRTYPE_CONNECT_TIMEOUT                     = 401,          // 连接对端超时
    ERRTYPE_CONNECT_CONNREFUSED                 = 402,          // 连接被拒绝
    ERRTYPE_CONNECT_SUCCESS                     = 403,          // 连接成功
    ERRTYPE_CONNECT_TRY_AGAIN                   = 404,          // 忙，请稍后重试
};

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
typedef int64_t ConnId;
// 事件id
typedef int64_t EventId;
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
typedef std::function<void(ConnId, ErrOpt)> OnConnectFunc;

} // namespace bbt::network

#define FASTERR(info, type) std::make_optional<Errcode>(info, type)
#define FASTERR_ERROR(info) FASTERR(BBT_NETWORK_MODULE info, bbt::network::emErr::ERRTYPE_ERROR)
#define FASTERR_NOTHING std::nullopt