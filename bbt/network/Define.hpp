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

#include <bbt/core/macroutil/Assert.hpp>
#include <bbt/core/Attribute.hpp>
#include <bbt/core/net/IPAddress.hpp>
#include <bbt/network/Errcode.hpp>

namespace bbt::network
{

using namespace bbt::core::errcode;
using namespace bbt::core::net;

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

// 连接id
typedef uint64_t ConnId;
// 事件id
typedef uint64_t EventId;

}

#define FASTERR(info, type) std::make_optional<Errcode>(info, type)
#define FASTERR_ERROR(info) FASTERR(info, bbt::network::ErrType::ERRTYPE_ERROR)
#define FASTERR_NOTHING std::nullopt