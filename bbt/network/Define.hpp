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

#include <event2/event.h>

namespace bbt::network
{

// 连接状态枚举
enum ConnStatus
{
    DEFAULT     = 0,    // 默认状态
    CONNECTED   = 1,    // 连接完成
    DECONNECTED = 2,    // 断开连接
};

}

#define FASTERR_ERROR(info) Errcode{info, ErrType::ERRTYPE_ERROR}

#define FASTERR_NOTHING Errcode{"", ErrType::ERRTYPE_NOTHING}