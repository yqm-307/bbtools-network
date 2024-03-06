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

#include <bbt/base/assert/Assert.hpp>
#include <bbt/base/Attribute.hpp>
#include <bbt/network/Errcode.hpp>

namespace bbt::network
{

// 空闲断开连接时间
#define CONNECTION_FREE_TIMEOUT_MS 5000
// 发送超时失败
#define SEND_DATA_TIMEOUT_MS 2000
// 连接超时
#define CONNECT_TIMEOUT_MS 2000

// 连接状态枚举
enum ConnStatus
{
    DEFAULT     = 0,    // 默认状态
    CONNECTED   = 1,    // 连接完成
    DECONNECTED = 2,    // 断开连接
};

// 连接id
typedef uint64_t ConnId;
// 事件id
typedef uint64_t EventId;

BBTATTR_COMM_Unused
static bool GlobalInit()
{
    return evthread_use_pthreads() == 0;
}

}

#define FASTERR_ERROR(info) Errcode(info, ErrType::ERRTYPE_ERROR)

#define FASTERR_NOTHING Errcode("", ErrType::ERRTYPE_NOTHING)