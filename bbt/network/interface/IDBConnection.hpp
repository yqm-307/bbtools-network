/**
 * @file IDBConnection.hpp
 * @author yangqingmiao
 * @brief 数据库连接接口类定义
 * @version 0.1
 * @date 2024-02-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <bbt/network/ICallback.hpp>

namespace bbt::network::interface
{

class IDBConnection:
    public IDBCallback
{
};

}