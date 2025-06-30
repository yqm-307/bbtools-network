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
#include <bbt/core/clock/Clock.hpp>
#include <bbt/core/thread/Lock.hpp>
#include <bbt/pollevent/Event.hpp>
#include <bbt/network/detail/Connection.hpp>

using namespace bbt::core::errcode;

namespace bbt::network::detail
{

typedef bbt::pollevent::EventOpt EventOpt;

ConnId Connection::GenerateConnId()
{
    static std::atomic_uint64_t _id = 0;    // 返回值从1开始，0是非法值
    return ++_id;
}

std::shared_ptr<Connection> Connection::Create(std::weak_ptr<EvThread> thread, evutil_socket_t socket, const IPAddress& ipaddr)
{
    return std::make_shared<Connection>(thread, socket, ipaddr);
}

Connection::Connection(std::weak_ptr<EvThread> thread, evutil_socket_t socket, const IPAddress& ipaddr):
    m_bind_thread(thread),
    m_socket_fd(socket),
    m_peer_addr(ipaddr),
    m_conn_status(ConnStatus::emCONN_CONNECTED),
    m_conn_id(GenerateConnId())
{
    Assert(m_socket_fd >= 0);
    Assert(m_conn_id > 0);
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

void Connection::SetOpt_Callbacks(const ConnCallbacks& callbacks)
{
    m_callbacks = callbacks;
}

void Connection::OnRecv(const char* data, size_t len)
{
    if (!m_callbacks.on_recv_callback) {
        OnError(Errcode{"on recv!, but no recv callback!", ERRTYPE_ERROR});
        return;
    }

    m_callbacks.on_recv_callback(shared_from_this(), data, len);
}

void Connection::OnSend(ErrOpt err, size_t succ_len)
{
    if (!m_callbacks.on_send_callback) {
        OnError(Errcode{"on send!, but no send callback!", ERRTYPE_ERROR});
        return;
    }

    m_callbacks.on_send_callback(shared_from_this(), err, succ_len);
}

void Connection::OnClose()
{
    if (!m_callbacks.on_close_callback) {
        OnError(Errcode{"on closed!, but no close callback!", ERRTYPE_ERROR});
        return;
    }

    m_callbacks.on_close_callback(GetConnId(), GetPeerAddress());
}

void Connection::OnTimeout()
{
    if (!m_callbacks.on_timeout_callback) {
        OnError(Errcode{"on timeout!, but no timeout callback!", ERRTYPE_ERROR});
        return;
    }
    m_callbacks.on_timeout_callback(shared_from_this());
}

void Connection::OnError(const Errcode& err)
{
    if (m_callbacks.on_err_callback) {
        m_callbacks.on_err_callback(m_conn_id, err);
    }
}

void Connection::Close()
{
    if (IsClosed())
        return;

    if (m_event)
        m_event->CancelListen();
    if (m_send_event)
        m_send_event->CancelListen();
    // if (ret != 0) OnError(Errcode{"event cancel listen failed!", ERRTYPE_ERROR});
    
    CloseSocket();
    SetStatus(ConnStatus::emCONN_DECONNECTED);
    OnClose();
}

bool Connection::IsConnected() const
{
    return (m_conn_status == ConnStatus::emCONN_CONNECTED);
}

bool Connection::IsClosed() const
{
    return (m_conn_status == ConnStatus::emCONN_DECONNECTED);
}

const IPAddress& Connection::GetPeerAddress() const
{
    return m_peer_addr;
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
    [weak_this](int fd, short events, EventId eventid){
        auto pthis = weak_this.lock();
        if (!pthis) return;
        pthis->OnEvent(fd, events);
    });

    int ret = m_event->StartListen(m_timeout_ms);
    Assert(ret == 0);
}

void Connection::OnEvent(evutil_socket_t sockfd, short event)
{
    if (event & EventOpt::READABLE) {
        /* 尝试读取套接字数据，如果对端关闭，一并关闭此连接 */
        auto err = Recv(sockfd);
        if (err.has_value()) OnError(err.value());
        if (err.has_value() && err.value().Type() == emErr::ERRTYPE_NETWORK_RECV_EOF)
            Close();
    } else if (event & EventOpt::TIMEOUT) {
        /* 当连接空闲超时时，直接通过用户注册的回调通知用户 */
        Timeout();
    } else if (event & EventOpt::CLOSE) {
        Close();
    }
}

ErrOpt Connection::Recv(evutil_socket_t sockfd)
{
    int                 err          = 0;
    int                 read_len     = 0;
    bbt::core::Buffer   buffer;
    char*               buffer_begin = nullptr;
    size_t              buffer_len   = 4096;
    ErrOpt errcode = std::nullopt;

    if (IsClosed()) {
        return FASTERR_ERROR("conn is closed, but event was not cancel! peer:" + GetPeerAddress().GetIPPort());
    }

    buffer.WriteNull(buffer_len);
    buffer_begin = buffer.Peek();
    read_len = ::read(sockfd, buffer_begin, buffer_len);

    if (read_len == -1) {
        if (errno == EINTR || errno == EAGAIN) {
            errcode = std::make_optional<Errcode>("please try again!", ERRTYPE_NETWORK_RECV_TRY_AGAIN);
        } else if (errno == ECONNREFUSED) {
            errcode = std::make_optional<Errcode>("connect refused!", ERRTYPE_NETWORK_RECV_CONNREFUSED);
        } else {
            errcode = std::make_optional<Errcode>("other errno! errno=" + std::to_string(errno), ERRTYPE_NETWORK_RECV_OTHER_ERR);
        }

    } else if (read_len == 0) {
        errcode = std::make_optional<Errcode>("peer connect closed!", ERRTYPE_NETWORK_RECV_EOF);
    } else if (read_len < -1) {
        errcode = std::make_optional<Errcode>("other error! please debug!", ERRTYPE_NETWORK_RECV_OTHER_ERR);
    }

    if (errcode.has_value())
        return errcode;

    OnRecv(buffer_begin, read_len);

    return FASTERR_NOTHING;
}

size_t Connection::Send(const char* buf, size_t len)
{
    int remain = len;
    while (remain > 0) {
        int n = ::send(GetSocket(), (buf + (len - remain)), remain, MSG_NOSIGNAL);
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

ErrOpt Connection::AsyncSend(const char* buf, size_t len)
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
        return FASTERR_ERROR("send error! connection is disconnect! sockfd=" + std::to_string(GetSocket()) +  " status=" + std::to_string(IsConnected() ? 1 : 0));
    }

    /**
     *  后续调用不可以注册发送事件，只能追写output buffer，除非
     *  上一个发送事件已经结束
     */
    bool not_free = true;
    int append_len = AppendOutputBuffer(buf, len);
    if (!m_output_buffer_is_free.compare_exchange_strong(not_free, false)) {
        return (append_len != len) ? FASTERR_ERROR("output buffer failed! remain=" + std::to_string(len - append_len)) : FASTERR_NOTHING;
    }

    /* 如果此时没有进行中的发送事件，则注册一个新的发送事件 */
    return RegistASendEvent();
}

int Connection::AppendOutputBuffer(const char* data, size_t len)
{
    std::lock_guard<bbt::core::thread::Mutex> lock(m_output_mutex);
    auto before_size = m_output_buffer.Size();
    m_output_buffer.WriteString(data, len);
    auto after_size = m_output_buffer.Size();

    int change_num = after_size - before_size;

    return change_num > 0 ? change_num : 0;
}

ErrOpt Connection::RegistASendEvent()
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
    auto buffer_sptr = std::make_shared<bbt::core::Buffer>();
    /* Swap 是无额外开销的 */
    {
        std::lock_guard<bbt::core::thread::Mutex> lock(m_output_mutex);
        buffer_sptr->Swap(m_output_buffer);
    }

    auto weak_this = weak_from_this();
    if (!BindThreadIsRunning())
        return FASTERR_ERROR("bind thread is stop!");
    
    auto thread = GetBindThread();
    if (thread == nullptr)
        return FASTERR_ERROR("bind thread is nullptr!");

    m_send_event = thread->RegisterEvent(GetSocket(), EventOpt::WRITEABLE | EventOpt::PERSIST,
    [weak_this, buffer_sptr](int fd, short events, EventId eventid){
        auto pthis = weak_this.lock();
        if (!pthis) return;
        pthis->OnSendEvent(buffer_sptr, events);
    });

    m_send_event->StartListen(SEND_DATA_TIMEOUT_MS);
    return FASTERR_NOTHING;
}

void Connection::OnSendEvent(std::shared_ptr<bbt::core::Buffer> output_buffer, short events)
{
    ErrOpt err = std::nullopt;
    int size = 0;

    if (IsClosed()) return;
    if (events & EventOpt::TIMEOUT) {
        err = std::make_optional<Errcode>("send timeout!", ERRTYPE_SEND_TIMEOUT);
    } else if (events & EventOpt::WRITEABLE) {
        size = Send(output_buffer->Peek(), output_buffer->Size());
    }

    OnSend(err, size);

    /* 当连接已经关闭后，也退出事件；
    有待发送数据，交换buffer，继续发送；否则取消监听事件，释放标志位 */
    if (IsClosed() || m_output_buffer.Size() <= 0) {
        m_send_event->CancelListen();
        m_send_event = nullptr;
        m_output_buffer_is_free.exchange(true); // 允许注册发送事件
    } else {
        std::lock_guard<bbt::core::thread::Mutex> lock(m_output_mutex);
        Assert(output_buffer->Size() >= 0);
        output_buffer->Swap(m_output_buffer);
        m_output_buffer.Clear();
    }
}


ErrOpt Connection::Timeout()
{
    OnTimeout();
    Close();
    return FASTERR_NOTHING;
}

std::shared_ptr<EvThread> Connection::GetBindThread()
{
    return m_bind_thread.lock();
}

bool Connection::BindThreadIsRunning()
{
    if (m_bind_thread.expired())
        return false;
    
    auto thread = m_bind_thread.lock();
    if (thread == nullptr)
        return false;
    
    return thread->IsRunning();
}

void Connection::CloseSocket()
{
    if (m_socket_fd < 0)
        return;

    ::close(m_socket_fd);
    m_socket_fd = -1;
}

void Connection::SetStatus(ConnStatus status)
{
    AssertWithInfo(status >= m_conn_status, "status can`t rollback!");  // 连接状态不允许回退
    m_conn_status = status;
}

ConnId Connection::GetConnId() const
{
    return m_conn_id;
}

evutil_socket_t Connection::GetSocket() const
{
    return m_socket_fd;
}


} // namespace bbt::network::libevent
