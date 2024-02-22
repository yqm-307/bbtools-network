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
};

class Errcode:
    public bbt::errcode::Errcode<ErrType>
{
public:
    explicit Errcode(const std::string& info, ErrType type = ErrType::ERRTYPE_ERROR)
        :bbt::errcode::Errcode<ErrType>(info, type)
    {
    }

    explicit Errcode(const Errcode& err)
        :bbt::errcode::Errcode<ErrType>(err)
    {
    }
    
    explicit Errcode(Errcode&& err)
        :bbt::errcode::Errcode<ErrType>(err)
    {
    }

    operator bool()
    { return (Type() == ErrType::ERRTYPE_NOTHING); }    

    ErrType Type() { return GetErrType(); }
    const std::string& What() { return GetMsg(); }
    const char* CWhat() { return GetCMsg(); }
};

}