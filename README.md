# bbtools-network

bbt network 模块 - 基于事件驱动的高性能网络库

## 概述

bbtools-network 是一个基于 libevent 的 C++ 网络库，提供了异步的 TCP 客户端和服务器实现。该库采用事件驱动的架构，支持高并发连接和异步 I/O 操作。

## 特性

- 🚀 **异步非阻塞**: 基于 libevent 的事件驱动架构
- 🔒 **线程安全**: 支持多线程环境下的并发操作
- 📦 **连接管理**: 自动管理连接生命周期和资源释放
- ⏰ **超时控制**: 支持连接超时和空闲超时配置
- 🔄 **重连机制**: 客户端支持自动重连功能
- 📊 **负载均衡**: 服务器支持多线程负载均衡

## 架构设计

### 核心模块

```
bbt/network/
├── TcpClient.hpp/.cc      # TCP客户端实现
├── TcpServer.hpp/.cc      # TCP服务器实现
└── detail/
    ├── Define.hpp         # 基础定义和类型
    └── Connection.hpp/.cc # 连接管理核心类
```

### 模块说明

#### 1. TcpClient - TCP客户端
[`TcpClient`](bbt/network/TcpClient.hpp) 提供异步TCP客户端功能：

- **连接管理**: 支持异步连接和同步连接
- **数据传输**: 异步发送和接收数据
- **重连机制**: 支持自动重连到服务器
- **事件回调**: 连接、断开、超时、数据收发事件回调

**主要接口**:
```cpp
// 异步连接
core::errcode::ErrOpt AsyncConnect(const IPAddress& addr, int timeout);
// 同步连接  
core::errcode::ErrOpt Connect(const IPAddress& addr, int timeout);
// 发送数据
core::errcode::ErrOpt Send(const bbt::core::Buffer& buffer);
// 设置回调
void SetOnConnect(const OnConnectFunc& on_connect);
void SetOnRecv(const OnRecvFunc& on_recv);
```

#### 2. TcpServer - TCP服务器
[`TcpServer`](bbt/network/TcpServer.hpp) 提供高性能TCP服务器功能：

- **多线程处理**: 支持线程池处理客户端连接
- **负载均衡**: 自动分配连接到不同工作线程
- **连接管理**: 自动管理所有客户端连接
- **批量操作**: 支持向指定连接发送数据

**主要接口**:
```cpp
// 启动监听
core::errcode::ErrOpt AsyncListen(const IPAddress& addr, const OnAcceptFunc& onaccept_cb);
// 发送数据到指定连接
core::errcode::ErrOpt Send(ConnId connid, const bbt::core::Buffer& buffer);
// 获取连接对象
detail::ConnectionSPtr GetConnection(ConnId connid);
```

#### 3. Connection - 连接管理
[`Connection`](bbt/network/detail/Connection.hpp) 是网络连接的核心实现：

- **事件处理**: 处理读、写、超时、关闭事件
- **缓冲管理**: 自动管理输入输出缓冲区
- **异步发送**: 支持异步数据发送和发送队列
- **状态管理**: 跟踪连接状态变化

#### 4. Define.hpp - 基础定义
[`Define.hpp`](bbt/network/detail/Define.hpp) 包含核心类型和常量定义：

```cpp
// 连接ID类型
typedef int64_t ConnId;
// 事件ID类型  
typedef int64_t EventId;
// 回调函数类型
typedef std::function<void(ConnId)> OnTimeoutFunc;
typedef std::function<void(ConnId)> OnCloseFunc;
typedef std::function<void(ConnId, core::errcode::ErrOpt, size_t)> OnSendFunc;
typedef std::function<void(ConnId, const bbt::core::Buffer&)> OnRecvFunc;
```

## 示例程序

### 1. 简单客户端 - [client.cc](example/client.cc)
展示基础的客户端连接和回调设置：

```cpp
auto client = std::make_shared<TcpClient>(evthread);
client->SetConnectionTimeout(1000);
client->SetOnConnect([](auto id, ErrOpt err){
    std::cout << "连接结果: " << (err.has_value() ? err->CWhat() : "成功") << std::endl;
});
client->AsyncConnect({"127.0.0.1", 11001}, 100);
```

### 2. 简单服务器 - [server.cc](example/server.cc)
展示基础的服务器监听和连接处理：

```cpp
auto server = std::make_shared<TcpServer>(evthread);
server->SetOnClose([](ConnId id){
    std::cout << "连接关闭: " << id << std::endl;
});
server->AsyncListen({"", 11001}, [](ConnId id){
    std::cout << "新连接: " << id << std::endl;
});
```

### 3. Echo服务器 - [echo_server.cc](example/echo_server.cc)
完整的回声服务器实现，展示数据收发处理。

### 4. 压力测试客户端 - [echo_client.cc](example/echo_client.cc)
高并发客户端测试程序：

- 支持创建多个并发连接
- 实时监控收发数据量
- 性能统计和报告

使用方法：
```bash
./echo_client <ip> <port> <client_count>
```

### 5. 事件线程示例 - [evthread.cc](example/evthread.cc)
展示事件循环和定时器的使用。

## 编译和安装

### 依赖要求
- CMake 3.10+
- C++17 兼容编译器
- libevent 2.0+
- bbtools-core 核心库

### 编译步骤

```bash
# 克隆仓库
git clone <repository-url>
cd bbtools-network

# 创建构建目录
mkdir build && cd build

# 配置和编译
cmake ..
make

# 安装
sudo make install
```

### 运行示例

```bash
# 运行服务器
./bin/example/test_server

# 运行客户端  
./bin/example/test_client

# 运行Echo服务器
./bin/example/echo_server <port>

# 运行压力测试
./bin/example/echo_client 127.0.0.1 <port> 100
```

## 许可证

本项目采用 Apache License 2.0 许可证，详见 [LICENSE](LICENSE) 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个库。

## 性能特点

- **高并发**: 单服务器支持数万并发连接
- **低延迟**: 基于事件驱动的零拷贝设计
- **内存高效**: 智能缓冲区管理和连接池
- **可扩展**: 支持多线程和负载均衡
