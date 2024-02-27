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
#include <bbt/base/errcode/Errcode.hpp>
#include <bbt/network/Define.hpp>

namespace bbt::network
{

enum ErrType
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
};

class Errcode:
    public bbt::errcode::Errcode<ErrType>
{
public:
    Errcode(const std::string& info, ErrType type = ErrType::ERRTYPE_ERROR)
        :bbt::errcode::Errcode<ErrType>(info, type)
    {
    }

    Errcode(const Errcode& err)
        :bbt::errcode::Errcode<ErrType>(err)
    {
    }
    
    Errcode(Errcode&& err)
        :bbt::errcode::Errcode<ErrType>(err)
    {
    }

    operator bool()
    { return (Type() == ErrType::ERRTYPE_NOTHING); }    

    ErrType     Type()
    { return GetErrType(); }
    
    const std::string& What()
    { return GetMsg(); }

    const char* CWhat()
    { return GetCMsg(); }

    void        SetInfo(const std::string& info)
    { SetMsg(info); }

    void        SetType(ErrType type)
    { SetErrType(type); }
};

}