/**
 * @file Errcode.hpp
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <bbt/core/errcode/Errcode.hpp>
#include <bbt/network/Define.hpp>

namespace bbt::network
{

using namespace bbt::core::errcode;

enum ErrType : bbt::core::errcode::ErrType
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

}