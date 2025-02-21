#include <bbt/network/adapter/libevent/Network.hpp>
#include <bbt/base/Logger/Logger.hpp>

using namespace bbt::network;
using namespace bbt::network::libevent;

class EchoServer
{
    struct UData{ConnId connid{0};};
public:
    EchoServer(int thread_num, const char* ip, short port)
    {
        m_network.AutoInitThread(thread_num);
        m_network.StartListen(ip, port, [this](auto err, auto sptr){
            if (!err.IsErr())
                OnNewConnection(sptr);
            else
                BBT_BASE_LOG_ERROR("[%s]", err.CWhat());
        });

        m_callback.on_close_callback = 
        [this](void* udata, const bbt::net::IPAddress& addr) {
            UData* data = reinterpret_cast<UData*>(udata);
            BBT_BASE_LOG_INFO("onclose [%d], %s", data->connid, addr.GetIPPort().c_str());
            m_conn_map.erase(data->connid);
        };

        m_callback.on_err_callback =
        [](void* udata, const bbt::errcode::Errcode& err){
            UData* data = reinterpret_cast<UData*>(udata);
            BBT_BASE_LOG_ERROR("[%d], %s", data->connid, err.CWhat());
        };

        m_callback.on_recv_callback =
        [](libevent::ConnectionSPtr sptr, const char* data, size_t len){
            BBT_BASE_LOG_DEBUG("onrecv succ, size=%d", len);
            sptr->AsyncSend(data, len);
        };

        m_callback.on_send_callback =
        [](libevent::ConnectionSPtr conn, const bbt::errcode::Errcode& err, size_t send_len){
            BBT_BASE_LOG_DEBUG("[%d] send succ=%d", conn->GetConnId(), send_len);
        };

        m_callback.on_timeout_callback =
        [](libevent::ConnectionSPtr conn){
            BBT_BASE_LOG_DEBUG("[%d] timeout", conn->GetConnId());
            conn->Close();
        };
    }

    ~EchoServer()
    {
    }

    void Start()
    {
        m_network.Start();
        BBT_BASE_LOG_INFO("server start!");
        while(true)
            sleep(1);
    }
protected:
    void InitConn(libevent::ConnectionSPtr conn)
    {
        UData* udata = new UData();
        udata->connid = conn->GetConnId();
        conn->SetOpt_UserData(udata);
        conn->SetOpt_Callbacks(m_callback);
    }

    void OnNewConnection(libevent::ConnectionSPtr conn)
    {
        BBT_BASE_LOG_INFO("a new connection! %s", conn->GetPeerAddress().GetIPPort().c_str());
        InitConn(conn);
        m_conn_map.insert(std::make_pair(conn->GetConnId(), conn));
    }
private:
    ConnCallbacks m_callback;
    Network m_network;
    std::map<ConnId, libevent::ConnectionSPtr> m_conn_map;
};

int main(int args, char* argv[])
{
    int console_debug_flag = 1;
    BBT_CONFIG_QUICK_SET_DYNAMIC_ENTRY(int, &console_debug_flag, bbt::config::BBT_LOG_STDOUT_OPEN);
    if (args != 4) {
        printf("[usage] ./{exec_name} {thread_num} {ip} {port}");
        exit(-1);
    }
    int     thread_num  = std::stoi(argv[1]);
    char*   ip          = argv[2];
    short   port        = (short)std::stoi(argv[3]);

    EchoServer server{thread_num, ip, port};


    server.Start();
}   