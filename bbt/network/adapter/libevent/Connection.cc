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
#include <bbt/base/timer/Clock.hpp>
#include <bbt/base/thread/Lock.hpp>
#include <bbt/network/adapter/libevent/Connection.hpp>
#include <bbt/network/adapter/libevent/IOThread.hpp>

namespace bbt::network::libevent
{

std::shared_ptr<Connection> Connection::Create(std::shared_ptr<libevent::IOThread> thread, evutil_socket_t socket, const bbt::net::IPAddress& ipaddr)
{
    return std::make_shared<Connection>(thread, socket, ipaddr);
}

Connection::Connection(std::shared_ptr<libevent::IOThread> thread, evutil_socket_t socket, const bbt::net::IPAddress& ipaddr)
    :LibeventConnection(thread, socket, ipaddr)
{
}

Connection::~Connection()
{
    Close();
}

void Connection::SetOpt_CloseTimeoutMS(int timeout_ms)
{
    AssertWithInfo(timeout_ms > 0, "timeout can`t less then 0!");
    m_timeout_ms = timeout_ms;
}

void Connection::SetOpt_Callbacks(const libevent::ConnCallbacks& callbacks)
{
    m_callbacks = callbacks;
}

void Connection::SetOpt_UserData(void* userdata)
{
    m_userdata = userdata;
}

void Connection::GetUserData(void* userdata)
{
    userdata = m_userdata;
}

void Connection::OnRecv(const char* data, size_t len)
{
    if (!m_callbacks.on_recv_callback) {
        OnError(Errcode{"on recv!, but no recv callback!"});
        return;
    }

        m_callbacks.on_recv_callback(shared_from_this(), data, len);
}

void Connection::OnSend(const Errcode& err, size_t succ_len)
{
    if (!m_callbacks.on_send_callback) {
        OnError(Errcode{"on send!, but no send callback!"});
        return;
    }

    m_callbacks.on_send_callback(shared_from_this(), err, succ_len);
}

void Connection::OnClose()
{
    if (!m_callbacks.on_close_callback) {
        OnError(Errcode{"on closed!, but no close callback!"});
        return;
    }

    m_callbacks.on_close_callback(m_userdata, GetPeerAddress());
}

void Connection::OnTimeout()
{
    if (!m_callbacks.on_timeout_callback) {
        OnError(Errcode{"on timeout!, but no timeout callback!"});
        return;
    }
    m_callbacks.on_timeout_callback(shared_from_this());
}

void Connection::OnError(const bbt::errcode::Errcode& err)
{
    if (m_callbacks.on_err_callback) {
        m_callbacks.on_err_callback(m_userdata, dynamic_cast<const Errcode&>(err));
    }
}

void Connection::Close()
{
    if (IsClosed())
        return;

    auto err = m_event->CancelListen();
    if (m_send_event)
        m_send_event->CancelListen();
    if (err.IsErr()) OnError(err);
    
    CloseSocket();
    SetStatus(ConnStatus::DECONNECTED);
    OnClose();
}

void Connection::RunInEventLoop()
{
    auto weak_this = weak_from_this();
    if (!BindThreadIsRunning())
        return;

    auto thread = GetBindThread();

    Assert(thread != nullptr);
    if (thread == nullptr)
        return;

    m_event = thread->RegisterEvent(GetSocket(),
        EventOpt::CLOSE |       // 关闭事件
        EventOpt::PERSIST |     // 持久化
        EventOpt::READABLE,     // 可读事件
    [weak_this](std::shared_ptr<Event> event, short events){
        auto pthis = weak_this.lock();
        if (!pthis) return;
        pthis->OnEvent(event->GetSocket(), events);
    });

    m_event->StartListen(m_timeout_ms);
}

void Connection::OnEvent(evutil_socket_t sockfd, short event)
{
    if (event & EventOpt::READABLE) {
        /* 尝试读取套接字数据，如果对端关闭，一并关闭此连接 */
        auto err = Recv(sockfd);
        if (err.IsErr()) OnError(err);
        if (err.Type() == ErrType::ERRTYPE_NETWORK_RECV_EOF)
            Close();
    } else if (event & EventOpt::TIMEOUT) {
        /* 当连接空闲超时时，直接通过用户注册的回调通知用户 */
        Timeout();
    } else if (event & EventOpt::CLOSE) {
        Close();
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
            errcode = Errcode{"please try again!", ERRTYPE_NETWORK_RECV_TRY_AGAIN};
        } else if (errno == ECONNREFUSED) {
            errcode = Errcode{"connect refused!", ERRTYPE_NETWORK_RECV_CONNREFUSED};
        } else {
            errcode = Errcode{"other errno! errno=" + std::to_string(errno), ERRTYPE_NETWORK_RECV_OTHER_ERR};
        }

    } else if (read_len == 0) {
        errcode = Errcode{"peer connect closed!", ERRTYPE_NETWORK_RECV_EOF};
    } else if (read_len < -1) {
        errcode = Errcode{"other error! please debug!", ERRTYPE_NETWORK_RECV_OTHER_ERR};
    }

    if (errcode.IsErr())
        return errcode;

    OnRecv(buffer_begin, read_len);

    return FASTERR_NOTHING;
}

size_t Connection::Send(const char* buf, size_t len)
{
    int remain = len;
    while (remain > 0) {
        int n = ::send(GetSocket(), (buf + (len - remain)), remain, MSG_NOSIGNAL);
        // int n = ::write(GetSocket(), (buf + (len - remain)), remain);
        if (n < 0) {
            if (errno == EPIPE) {
                Close();
                return -1;
            }
        }
        remain -= n;
    }

    return (len - remain);
}

int Connection::AsyncSend(const char* buf, size_t len)
{
    /**
     *  此函数大概率是跨线程发送的，因此内部保证线程安全
     *  异步发送数据时有下列情况：
     *  （1）当有正在发送中的数据，则将数据追加到输出缓存中
     *  （2）当没有发送中的数据，则触发一次发送数据
     * 
     *  同时在发送事件完成后，会检测output buffer中是否有
     *  待发送数据，如果有，则继续上述循环直到buffer为空.
     */
    if (!IsConnected()) {
        std::string info = bbt::log::format("send error! connection is disconnect! sockfd=%d, status=%d", GetSocket(), IsConnected() ? 1 : 0);
        OnError(FASTERR_ERROR(info));
        return -1;
    }

    /**
     *  后续调用不可以注册发送事件，只能追写output buffer，除非
     *  上一个发送事件已经结束
     */
    bool not_free = true;
    int append_len = AppendOutputBuffer(buf, len);
    if (!m_output_buffer_is_free.compare_exchange_strong(not_free, false)) {
        return (append_len != len) ? (-1) : (0);
    }

    /* 如果此时没有进行中的发送事件，则注册一个新的发送事件 */
    return RegistASendEvent();
}

int Connection::AppendOutputBuffer(const char* data, size_t len)
{
    bbt::thread::lock_guard lock(m_output_mutex);
    auto before_size = m_output_buffer.DataSize();
    m_output_buffer.WriteString(data, len);
    auto after_size = m_output_buffer.DataSize();

    int change_num = after_size - before_size;

    return change_num > 0 ? change_num : 0;
}

int Connection::RegistASendEvent()
{
    /**
     *  此事件只同时存在一个，其作用是每次发送结束后，检测一下
     *  输出缓存是否会有数据，并做以下行为：
     *      （1）output buffer 有数据，交换buffer，该事件继续运行；
     *      （2）output buffer 没有数据，取消此事件，允许下次追加output
     *          buffer时注册发送事件.
     */
    AssertWithInfo(!m_output_buffer_is_free.load(), "output buffer must be false!");
    AssertWithInfo(!m_send_event , "has a wrong!");
    auto buffer_sptr = std::make_shared<bbt::buffer::Buffer>();
    /* Swap 是无额外开销的 */
    {
        bbt::thread::lock_guard lock(m_output_mutex);
        buffer_sptr->Swap(m_output_buffer);
    }

    auto weak_this = weak_from_this();
    if (!BindThreadIsRunning())
        return -1;
    
    auto thread = GetBindThread();
    if (thread == nullptr)
        return -1;

    m_send_event = thread->RegisterEvent(GetSocket(), EventOpt::WRITEABLE | EventOpt::PERSIST,
    [weak_this, buffer_sptr](std::shared_ptr<Event> event, short events){
        auto pthis = weak_this.lock();
        if (!pthis) return;
        pthis->OnSendEvent(buffer_sptr, event, events);
    });

    m_send_event->StartListen(SEND_DATA_TIMEOUT_MS);
    return 0;
}

void Connection::OnSendEvent(std::shared_ptr<bbt::buffer::Buffer> output_buffer, std::shared_ptr<Event> event, short events)
{
    Errcode     err{"", ErrType::ERRTYPE_NOTHING};
    int         size = 0;

    if (IsClosed()) return;
    if (events & EventOpt::TIMEOUT) {
        err = Errcode{"send timeout!", ERRTYPE_SEND_TIMEOUT};
    } else if (events & EventOpt::WRITEABLE) {
        size = Send(output_buffer->Peek(), output_buffer->DataSize());
    }

    OnSend(err, size);

    /* 当连接已经关闭后，也退出事件；
    有待发送数据，交换buffer，继续发送；否则取消监听事件，释放标志位 */
    if (IsClosed() || m_output_buffer.DataSize() <= 0) {
        m_send_event->CancelListen();
        m_send_event = nullptr;
        m_output_buffer_is_free.exchange(true); // 允许注册发送事件
    } else {
        bbt::thread::lock_guard lock(m_output_mutex);
        Assert(output_buffer->DataSize() >= 0);
        output_buffer->Swap(m_output_buffer);
    }
}


Errcode Connection::Timeout()
{
    OnTimeout();
    Close();
    return FASTERR_NOTHING;
}

} // namespace bbt::network::libevent
