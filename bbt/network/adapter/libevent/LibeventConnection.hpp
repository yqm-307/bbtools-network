#pragma once
#include <bbt/network/adapter/base/Connection.hpp>

namespace bbt::network::libevent
{

class IOThread;

class LibeventConnection:
    public bbt::network::base::ConnectionBase
{
public:
    LibeventConnection(std::shared_ptr<libevent::IOThread> bind_thread, int socket, const IPAddress& addr);

    /**
     * @brief 获取当前连接所绑定的线程，失败返回nullptr
     * 
     * @return std::shared_ptr<libevent::IOThread> 
     */
    std::shared_ptr<libevent::IOThread> GetBindThread();

    /**
     * @brief 获取绑定线程是否运行中
     * 
     * @return true 
     * @return false 
     */
    bool BindThreadIsRunning();
private:
    /* 当前连接所绑定的线程，连接的IO操作将运行在bind线程中 */
    std::weak_ptr<libevent::IOThread> m_bind_thread;
};

}