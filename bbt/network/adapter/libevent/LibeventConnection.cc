#include <bbt/network/adapter/libevent/LibeventConnection.hpp>
#include <bbt/network/adapter/libevent/IOThread.hpp>

namespace bbt::network::libevent
{

LibeventConnection::LibeventConnection(std::shared_ptr<libevent::IOThread> bind_thread, int socket, const bbt::net::IPAddress& addr)
    :base::ConnectionBase(socket, addr), m_bind_thread(bind_thread) 
{
}


std::shared_ptr<libevent::IOThread> LibeventConnection::GetBindThread()
{
    return m_bind_thread.lock();
}

bool LibeventConnection::BindThreadIsRunning()
{
    if (m_bind_thread.expired())
        return false;
    
    auto thread = m_bind_thread.lock();
    if (thread != nullptr)
        return false;
    
    return thread->IsRunning();
}

}