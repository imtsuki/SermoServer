# SermoServer

Backend of Sermo, a word game.

This is part of the final project of course: Object-Oriented Programming Using C++.


## 设计亮点

- 完全的**面向对象设计**，同时采用**工厂模式**避免裸 `new`、`delete`，采用**单例模式**实现资源的安全共享；
- 服务端采用**多线程**实现**并发**处理，并采用**线程安全**的**消息队列**机制实现线程间的通信，并减少不必要的开销，同时经过少量修改即可支持**分布式架构**；
- 得益于消息机制，采用**回调函数注册机制**来进行所有游戏逻辑的处理，将函数也**视为对象**，同时实现了系统的**可拓展性**，易于增加游戏功能；
- 采用**长连接 TCP socket**，实现客户端/服务端**双向即时消息推送**；
- 采用 **JSON** 作为沟通格式约定，实现**前后端的分离**；同时便于统一传递错误信息（登录失败等等），增强了程序的**健壮性**；
- 采用 **SQLite 3** 存储数据到**数据库**中；
- 采用 **libconfig** 实现**变量**的**配置文件化**，**避免**了代码中的**全局常量/变量**，且无需重新编译便可由用户轻松修改相关数据；
- 客户端采用 Qt 进行编程，使用 QML 描述界面，C++ 编写逻辑；
- 实现了**多人对战**的附加功能，所有游戏界面均为亲自**手绘**。

## Requirements

- libconfig++
- libsqlite3
- g3logger

## Build

### Linux/macOS

```bash
$ git clone https://github.com/imtsuki/SermoServer.git
$ cd SermoServer/
$ make
$ ./build/server
```

## 服务器端程序设计

### 项目文件夹结构

- `include` 目录下是所有的头文件；
- `src` 目录下是所有的 `.cpp` 源文件；
- 根目录下，有 `Makefile` 文件；
- `sermo.db` 为数据库文件；
- `sermo.conf` 为配置文件。

### `main.cpp`：程序入口点

- 读取配置文件，初始化日志库；
- 实例化 `Server` 类，启动服务器，监听端口。

### `Server.h/cpp`：服务器类

`Server` 类采用**单例模式**，创建侦听套接字，并分配客户端线程进入事件循环。

开始时，调用静态成员函数 `getInstance()` 获取 `Server` 类实例；`getInstance()` 函数内部检查静态成员指针变量 `sharedInstance` 是否已经创建。若已创建，直接返回；若未创建，则调用 `createSharedInstance()` 创建该类实例。

```c++
bool Server::createSharedInstance() {
    sharedInstance = new (std::nothrow) Server();
    sharedInstance->init();
    return true;
}
```

>  注意到 `new` 运算符在失败时会抛出 `std::bad_alloc` 异常而不是返回空指针（返回值是未定义行为）。因此我们调用 `new` 的重载版本：`new (std::nothrow)` 来获得返回空指针的预期行为。

将构造函数与初始化代码分开是一个好的实践。我们将构造函数声明为 `private` 的默认构造。同时禁用拷贝构造与赋值操作符以防止单例的拷贝：

```c++
private:
		Server() = default;
		Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
```

`init` 函数主要做了以下工作：

- 创建侦听套接字
- 连接数据库
- 注册游戏业务逻辑

其中后两者我们在后面的章节进行讨论。

`createListenSocket()` 创建标准 Berkeley 套接字，将文件描述符存储在成员变量中。

```c++
// 为了清晰，省略了一些错误处理的逻辑，下同
listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
// 设置 SO_REUSEADDR 地址复用，让内核忽略处于 TIME_WAIT 状态的 socket 冲突
int on = 1;
result = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
```

之后，我们调用 `listenOn(int)` 绑定到指定端口，开始监听，并进入主循环。 

```c++
void Server::listenOn(int port) {
    bindToPort(port);
    listen(listenSocket, Config::getInstance()->lookup("server.backlog"));
    startMainLoop();
}
```

主循环不断地阻塞在 `accept` 调用，接受入境请求并分配客户端线程：

```c++
void Server::startMainLoop() {
    while (true) {
        // 接受入境请求
        acceptConnection(&clientSocket, &clientAddress);
        // 分配客户端线程
        dispatchClient(clientSocket, clientAddress);
    }
}
```

`dispatchClient()` 为当前连接创建一个**会话**对象（`Session`），创建并分离一个客户端线程，并立即返回。

```c++
void Server::dispatchClient(int clientSocket, sockaddr_in clientAddress) {
    auto session = Session::create(clientSocket, clientAddress);
    this->registerSession(session);

    std::thread child(&Server::startSessionEventLoop, this, session);
  
    child.detach();
    threads.push_back(std::move(child)); // std::thread 具有移动语义
}
```

新的线程从 `startSessionEventLoop()` 开始执行。而此函数立即创建另一个新线程来处理网络请求：

```c++
std::thread receiver(&Server::tcpMessageReceiver, this, session);
receiver.detach();
```

该新线程从 socket 中读取字符流，分辨信息边界（我们用 `\0` 字符来确定边界），打包为 Json 对象并推入消息队列。

```c++
session->messageQueue.pushMessage(Message(Message::MSGTYPE::REQUEST, body));
```

我们回到 `startSessionEventLoop()`。主要的结构依然是一个循环：

```c++
while (true) {
    auto message = session->messageQueue.popMessage();
    处理消息;
}
```

由此实现处理客户端请求。

### `Db.h/cpp`：数据库类

数据库类也采用**单例模式**。数据库内部实现采用 SQLite 3，并对外**隐藏**实现细节，实现**封装**。

`connect()` 函数中，有如下配置：

```c++
sqlite3_config(SQLITE_CONFIG_SERIALIZED);
```

配置为**串行**模式，保证**多线程**下的安全。

### `User.h/Player.h/Examiner.h`：玩家/出题者类及其父类

游戏中有两种角色：玩家与出题者。将二者分别抽象为 `Player` 与 `Examiner` 类；同时，二者有着共同的信息（用户名，密码等），因此将此部分抽象为二者共同继承的父类：`User` 类。

类中的数据字段全部为 `private`；采用 getter/setter 模式对属性进行封装。

同时，数据库用户信息相关的表设计也与这三个类一一对应：`user`、`player`、`examiner` 三个表。

以下是数据库的相关表结构：

```sqlite
CREATE TABLE user(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    usertype INTEGER NOT NULL DEFAULT 0,
    username TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
    nickname TEXT,
    avatar INTEGER,
    level INTEGER DEFAULT 1
);

CREATE TABLE player(
    id INTEGER PRIMARY KEY,
    exprience INTEGER DEFAULT 0,
    achievement INTEGER DEFAULT 0
);

CREATE TABLE examiner(
    id INTEGER PRIMARY KEY,
    number_word INTEGER DEFAULT 0
);
```

### `GameLogic.cpp`：所有的游戏业务逻辑

本服务器端的游戏逻辑采取**动态注册**的机制。

在 `Server` 类中，有这样的一个函数：

```c++
void Server::registerGameLogic(std::string type, std::function<void (Json& response, Json& request)> callback) {
    callbackMap[type] = callback;
}
```

`callbackMap` 是一个从字符串到 lambda 函数对象的映射。这个函数接受以上两者，将映射关系存储在 `callbackMap` 里。

当客户端向服务器发送请求的时候，请求字段会附加这样的一个头部：

```json
{ "type": "connect" }
```

服务器接收到请求后，根据头部的 `type` 字段在 `callbackMap` 中寻找相关处理函数，调用并处理：

```c++
callbackMap[request["type"]](response, request);
```

而每个游戏逻辑，都在 `GameLogic.cpp` 中这样书写：

```c++
registerGameLogic(
    "connect",
    [&](Json& response, Json& request) {
        response["status"] = "success";
    }
);
```

这里采用了 **C++11** 新增的 **lambda 表达式**的特性。

### `Session.h`：抽象一次连接的「会话」类

服务器每当开启一个连接，就会创建一个 `Session` 对象，把所有相关信息都存储到这个会话里。

会话创建时，会生成一个独一无二的标识符：`sessionId`，客户端与服务器端都进行记录，在传递每条请求时都进行附加。

服务器端也有一个 `std::map<int, Session *> sessionMap` ，来进行 `sessionId` 和 `session` 的映射，以方便服务器查找信息。

每个 `Session` 对象均含有一个**消息队列**。其他线程均可以对这个会话进行**消息投递**。

```c++
struct Session {
    enum Status {CONNECTED, LOGIN, PVE, DISCONNECTED, DIED}; // 状态枚举
    SessionId id;								// 会话唯一标识符
    Status status;							// 会话状态
    User *user;									// 用户信息
  	// ...
    MessageQueue messageQueue;	// 消息队列
  	// ...
}
```

### `Message.h/MessageQueue.h`：消息/消息队列类

消息队列封装了 STL 的队列容器 `std::queue`，因为标准库的队列操作不是**原子**的。通过**锁机制**，实现了**线程安全**的读与写，让不同线程之间的沟通成为了可能。

```c++
void pushMessage(Message msg) {
    std::unique_lock<std::mutex> lock(accessLock);
  
    queue.push(msg);
  
    avaliable.notify_all();
}

Message popMessage() {
    std::unique_lock<std::mutex> lock(accessLock);
    avaliable.wait(lock, [&]() { return !queue.empty(); });
  
    auto ret = queue.front();
    queue.pop();
    return ret;
}
```

实验中，开启多个线程同时对同一个队列进行无延迟读写，没有出现**竞争条件**。

### `Config.h/log.h`：用来初始化或包装 libconfig 和 g3log 库的类

一些简单的封装，不再赘述。

## 请求格式

以下是客户端-服务器请求格式的约定。

报文采用 JSON 格式交换信息。头部不附带长度字段，而是采用 `\0` 来自然断开每个独立的报文。

请求与响应的结构如下：

```json
//请求：
{
    type, // 请求类型
    body, // 请求主体
    sessionId // 会话标识
}
//响应
{
    status,	// 响应状态
    body,		// 响应主体
    sessionId // 会话标识
}
```

一些请求的示例：

```json
{"type":"connect"}

{"type":"login","body":{"username":"tsuki","password":"123456"},"sessionId":1}
{"type":"login","body":{"username":"examiner","password":"456"},"sessionId":1}

{"type":"info","sessionId":1}

{"type":"playerList","sessionId":1}

{"type":"register","body":{"username":"player","password":"123","userType":0},"sessionId":1}
{"type":"register","body":{"username":"examiner","password":"456","userType":1},"sessionId":1}

{"type": "addWord","body":{"word":"awesome"},"sessionId":1}

{"type": "pve", "body":{"round":23},"sessionId":1}
{"type": "pveResult", "body":{"result":"passed","round": 23, "exp":300},"sessionId":1}
{"type": "pveResult", "body":{"result":"failed","round": 23},"sessionId":1}
```

## 客户端程序设计

客户端主要采用 Qt 的 QML 进行游戏界面的定义`*.qml`；由于主要的游戏逻辑均在服务器端进行运算，因而客户端的代码基本上为请求的处理、发送与用户展示。
