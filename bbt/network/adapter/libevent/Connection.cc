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
#include <string>
#include <bbt/base/buffer/Buffer.hpp>
#include <bbt/base/Logger/Logger.hpp>
#include <bbt/network/adapter/libevent/Connection.hpp>

namespace bbt::network::libevent
{

Connection::Connection(EventLoop* loop, evutil_socket_t socket, bbt::net::IPAddress& ipaddr)
    :ConnectionBase(socket, ipaddr),
    m_eventloop(loop)
{
    loop->CreateEvent(socket, EV_CLOSED | EV_PERSIST | EV_READ, 
    [this](evutil_socket_t socket, short events){
        OnEvent(socket, events);
    });
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

void Connection::SetCallbacks(const libevent::ConnCallbacks& cb)
{
    
}

void Connection::OnEvent(evutil_socket_t sockfd, short event)
{
    if (sockfd & EventOpt::READABLE) {
        auto err = Recv(sockfd);
        if (!err) OnError(err);
    // } else if (sockfd & EventOpt::WRITEABLE) {
        // OnSend();
    } else if (sockfd & EventOpt::TIMEOUT) {
        OnTimeout();
    } else if (sockfd & EventOpt::FD_CLOSE) {
        OnClose();
    }
}

Errcode Connection::Recv(evutil_socket_t sockfd)
{
    int                 err          = 0;
    int                 read_len     = 0;
    bbt::buffer::Buffer buffer;
    char*               buffer_begin = buffer.Peek();
    size_t              buffer_len   = buffer.WriteableBytes();
    Errcode             errcode{"nothing", ErrType::ERRTYPE_NOTHING};

    if (IsClosed()) {
        return FASTERR_ERROR("conn is closed, but event was not cancel! peer:" + GetPeerAddress().GetIPPort());
    }

    read_len = ::read(sockfd, buffer_begin, buffer_len);
    buffer.WriteNull(read_len);

    if (read_len == -1) {
        if (errno == EINTR || errno == EAGAIN) {
            errcode.SetInfo("please try again!");
            errcode.SetType(ErrType::ERRTYPE_NETWORK_RECV_TRY_AGAIN);
        } else if (errno == ECONNREFUSED) {
            errcode.SetInfo("connect refused!");
            errcode.SetType(ErrType::ERRTYPE_NETWORK_RECV_CONNREFUSED);
        }
    } else if (read_len == 0) {
        errcode.SetInfo("peer connect closed!");
        errcode.SetType(ErrType::ERRTYPE_NETWORK_RECV_EOF);
    } else if (read_len < -1) {
        errcode.SetInfo("other error! please debug!");
        errcode.SetType(ErrType::ERRTYPE_NETWORK_RECV_OTHER_ERR);
    }

    if (!errcode)
        return errcode;

    OnRecv(buffer_begin, buffer_len);
}

size_t Connection::Send(const char* buf, size_t len)
{
    int remain = len;
    while (remain > 0) {
        int n = ::write(GetSocket(), (buf + (len - remain)), remain);
        if (n < 0) {
            if (errno == EPIPE) {
                Close();
                return -1;
            }
            remain -= n;
        }
    }

    return (len - remain);
}

int Connection::AsyncSend(const char* buf, size_t len)
{
    /**
     *  异步发送数据
     *  （1）当有正在发送中的数据，则将数据追加到输出缓存中
     *  （2）当没有发送中的数据，则触发一次发送数据
     */
    if (!IsConnected()) {
        std::string info = bbt::log::format("send error! connection is disconnect! sockfd=%d, status=%d", GetSocket(), IsConnected() ? 1 : 0);
        OnError(FASTERR_ERROR(info));
        return -1;
    }

    bbt::thread::lock::lock_guard<bbt::thread::lock::Mutex> lock(m_output_mutex);

    if (!m_output_buffer_is_free) {
        int append_len = AppendOutputBuffer(buf, len);
        if (append_len != len) {
            return -1;
        }
    }

    AsyncSendInThread();
    return 0;
}

int Connection::AsyncSendInThread()
{
    /**
     *  每次发送结束后，回检测一下输出缓存是否会有数据。
     *  如果有再次注册一个send事件
     */
    bbt::buffer::Buffer buffer;
    m_output_buffer.Swap(buffer);

    size_t m_output_prev_size = buffer.DataSize();

    auto weak_this = weak_from_this();
    auto connid = GetMemberId();
    m_send_event = m_eventloop->CreateEvent(GetSocket(), EV_WRITE,
    [this, buffer](evutil_socket_t fd, short events){
        Errcode     err{"", ErrType::ERRTYPE_NOTHING};
        int         size = 0;

        size = Send(buffer.Peek(), buffer.DataSize());

        if (events & EventOpt::TIMEOUT) {
            err.SetType(ErrType::ERRTYPE_SEND_TIMEOUT);
            err.SetInfo("send timeout!");
        } else if (!(events & EventOpt::WRITEABLE)) {
            err.SetType(ErrType::ERRTYPE_ERROR);
            err.SetInfo("invalid event!");
        }

        OnSend(err, size);

        bbt::thread::lock::lock_guard<bbt::thread::lock::Mutex> lock(m_output_mutex);
        DebugAssert(!m_output_buffer_is_free);

        if (m_output_buffer.DataSize() > 0)
            AsyncSendInThread();
        else
            m_output_buffer_is_free = true;
    });

    m_output_buffer_is_free = false;

    m_send_event->StartListen(5000);
    return ;
}


Errcode Connection::Timeout()
{

}

} // namespace bbt::network::libevent
