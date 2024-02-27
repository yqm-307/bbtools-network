/**
 * @file Network.cc
 * @author yangqingmiao
 * @brief 
 * @version 0.1
 * @date 2024-02-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <bbt/network/adapter/libevent/Network.hpp>

namespace bbt::network::libevent
{

Network::Network(EventBase* io_context, uint32_t sub_thread_num, const char* ip, short port)
    :m_listen_addr(ip, port),
    m_io_context(io_context),
    m_sub_loop_nums(sub_thread_num)
{
    AssertWithInfo(m_io_context != nullptr, "io_context can`t NULL!");
}

Network::~Network()
{
}

libevent::ConnectionSPtr Network::NewConn(int fd)
{
    libevent::ConnectionSPtr            conn_sptr = nullptr;

    conn_sptr = Network::Create<libevent::Connection>(
        m_io_context,
        fd,
        );

    return
}

Errcode Network::Listen(const OnAcceptCallback& onaccept_cb)
{
    auto func = [this, onaccept_cb](evutil_socket_t fd, short events){
        if ( (events & EventOpt::READABLE) > 0 ) {
            auto conn_sptr = DoAccept(fd);
            onaccept_cb(conn_sptr);
        }
    };

    // m_onaccept_handle = onaccept_cb;
    m_onaccept_event = std::make_shared<Event>(
        m_io_context,
        m_listen_fd,
        EventOpt::READABLE | EventOpt::PERSIST,
        func);
}

void Network::OnError(const Errcode& err)
{
    if (m_error_handle != nullptr)
        m_error_handle(err);
}

void Network::SetOnErrorHandle(const OnErrorCallback& onerror_cb)
{
    m_error_handle = onerror_cb;
}

libevent::ConnectionSPtr Network::DoAccept(int listenfd)
{
    evutil_socket_t         newfd = -1;
    struct sockaddr_in      addr;
    socklen_t               len = sizeof(addr);
    bbt::net::IPAddress     endpoint;

    newfd = ::accept(listenfd, reinterpret_cast<sockaddr*>(&addr), &len);

    if ( (newfd < 0) && !(errno == EINTR ||  errno == EAGAIN || errno == ECONNABORTED) ) {
        OnError(FASTERR_ERROR("::accept() failed!"));
        return nullptr;
    }

    endpoint.set(addr);

    Create<libevent::Connection>(
        m_io_context,
        newfd,
        endpoint,
        m_conn_init_callbacks
    );


}


} // namespace bbt::network::libevent