/**
 * @file Connection.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */
// #include <bbt/network/adapter/libevent/Connection.hpp>
#include <Connection.hpp>

namespace bbt::network::libevent
{

Connection::Connection(EventBase* base, evutil_socket_t socket, bbt::net::IPAddress& ipaddr, const ConnCallbacks& callbacks)
    :ConnectionBase(socket, ipaddr),
    m_io_context(base),
    m_callbacks(callbacks)
{
    // m_event = std::make_shared(base, socket, EV_CLOSED | EV_PERSIST | EV_READ | EV_WRITE, [](

    // ){});
}

Connection::~Connection()
{
}

void Connection::OnRecv(const char* data, size_t len)
{
    if (m_callbacks.on_recv_callback) {
        m_callbacks.on_recv_callback(shared_from_this(), data, len);
    }
}

void Connection::OnSend(const Errcode& err, size_t succ_len)
{
    if (m_callbacks.on_send_callback) {
        m_callbacks.on_send_callback(err, succ_len);
    }
}

void Connection::OnClose()
{
    if (m_callbacks.on_close_callback) {
        m_callbacks.on_close_callback();
    }
}

void Connection::OnTimeout()
{
    if (m_callbacks.on_timeout_callback) {
        m_callbacks.on_timeout_callback();
    }
}

void Connection::OnError(const Errcode& err)
{
    if (m_callbacks.on_err_callback) {
        m_callbacks.on_err_callback(err);
    }
}

void Connection::Close()
{
    // 1、反注册事件
    // 2、关闭连接
    // 3、释放资源
}

void Connection::RegistEvent()
{
    // 1、申请内存
    // 2、初始化事件
    // 3、注册事件
}

void Connection::UnRegistEvent()
{
    // 1、注销事件
    // 2、释放内存
}


} // namespace bbt::network::libevent
