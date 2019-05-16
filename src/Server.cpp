#include "Server.h"
#include "Config.h"
#include "log.h"
#include "Db.h"
#include "MessageQueue.h"
#include <g3log/g3log.hpp>
#include <nlohmann/json.hpp>
#include <random>

using Json = nlohmann::json;

namespace Sermo {

Server *Server::sharedInstance = nullptr;

Server *Server::getInstance() {
    if (!sharedInstance) {
        createSharedInstance();
    }
    return sharedInstance;
}

bool Server::createSharedInstance() {
    sharedInstance = new (std::nothrow) Server();
    sharedInstance->init();
    return true;
}

void Server::init() {
    registerAllGameLogic();
    auto db = Db::getInstance();
    db->connect(Config::getInstance()->lookup("database.file"));
    createListenSocket();
}

void Server::listenOn(int port) {
    bindToPort(port);
    listen(listenSocket, Config::getInstance()->lookup("server.backlog"));
    LOG(INFO) << "Started.";
    LOG(INFO) << "Listening on " <<inet_ntoa(listenAddress.sin_addr) << ":" << port;

    startMainLoop();
}

void Server::createListenSocket() {
    int result;

    if (listenSocket) {
        result = close(listenSocket);
        if (result < 0) {
            LOG_WARNING_ERRNO();
        }
    }
    // Create listening socket
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket < 0) {
        LOG_FATAL_ERRNO();
    }
    // Set SO_REUSEADDR to disable TIME_WAIT
    int on = 1;
    result = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (result < 0) {
        LOG_WARNING_ERRNO();
    }
}

void Server::bindToPort(int port) {
    listenPort = port;
     // Set up listening socket
    listenAddress.sin_family = AF_INET;
    listenAddress.sin_addr.s_addr = inet_addr(Config::getInstance()->lookup("server.address"));
    // 注意字节序
    listenAddress.sin_port = htons(listenPort);
    // Bind to listening port
    int result = bind(listenSocket, (struct sockaddr *)&listenAddress, sizeof(listenAddress));
    if (result < 0) {
        LOG_FATAL_ERRNO();
    }
}

void Server::startMainLoop() {
    while (true) {
        int clientSocket;
        sockaddr_in clientAddress;
        acceptConnection(&clientSocket, &clientAddress);
        if (clientSocket < 0) {
            continue;
        }
        dispatchClient(clientSocket, clientAddress);
    }
}

void Server::acceptConnection(int *clientSocket, sockaddr_in *clientAddress) {
    socklen_t clientAddressSize = sizeof(clientAddress);
    memset(clientAddress, 0, sizeof(*clientAddress));

    *clientSocket = accept(listenSocket, (sockaddr *)clientAddress, &clientAddressSize);
    if (*clientSocket < 0) {
        if (errno == EINTR || errno == ECONNABORTED) {
            LOG_WARNING_ERRNO();
        } else {
            LOG_FATAL_ERRNO();
        }
        return;
    }

    LOG(INFO) << "Open ClientSocket " << inet_ntoa(clientAddress->sin_addr) << ":" << ntohs(clientAddress->sin_port);
}

void Server::dispatchClient(int clientSocket, sockaddr_in clientAddress) {
    auto session = Session::create(clientSocket, clientAddress);
    this->registerSession(session);

    std::thread child(&Server::startSessionEventLoop, this, session);

    LOG(INFO) << "Dispatch client " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << " to thread " << child.get_id();
    child.detach();
    threads.push_back(std::move(child));
}

void Server::registerSession(Session *session) {
    //static std::random_device rd;
    static int i = 1;
    //SessionId id = rd();
    session->id = i;
    sessionMap[i] = session;
    i++;
}

void Server::tcpMessageReceiver(Session *session) {
    int result;
    std::string body = "";
    while (true) {
        char c;
        result = read(session->clientSocket, &c, 1);
        if (result != 1) {
            LOG_WARNING_ERRNO();
            LOG(INFO) << "ClientSocket " << inet_ntoa(session->clientAddress.sin_addr) << ":" << ntohs(session->clientAddress.sin_port)
                      << " was closed by client.";
            close(session->clientSocket);
            session->messageQueue.pushMessage(Message(Message::MSGTYPE::CLOSE));
            break;
        }
        if (c == '\0') {
            LOG(INFO) << "Recieved client request: " << "\e[90m" << body << "\e[0m";
            session->messageQueue.pushMessage(Message(Message::MSGTYPE::REQUEST, body));
            body = "";
        } else {
            body += c;
        }
    }
    LOG(INFO) << "tcpMessageReceiver finished.";
}

void Server::startSessionEventLoop(Session *session) {
    // 创建 TCP 消息接收线程
    std::thread receiver(&Server::tcpMessageReceiver, this, session);
    LOG(INFO) << "Created tcpMessageReceiver " << receiver.get_id();
    receiver.detach();

    // 消息循环
    while (true) {
        // 从消息队列获取消息
        auto message = session->messageQueue.popMessage();
        if (message.type == Message::MSGTYPE::CLOSE) {
            // 清理资源
            if (session->user) {
                // 用户下线
                onlineSet.erase(session->user->getId());
                delete session->user;
                delete session;
            }
            break;
        }

        if (message.type == Message::MSGTYPE::NOTIFICATION) {
            auto& msg = message.body;
            std::string response;
            if (msg["msg"] == "invite") {
                session->opponent = onlineSet[msg["opponentId"]];
                session->pvpWord = session->opponent->pvpWord;
                response = R"({"type": "invite"})";
            } else if (msg["msg"] == "opponentAccepted") {
                Json json = {
                    {"type", "opponentAccepted"},
                    {"pvpWord", session->pvpWord}
                };
                response = json.dump();
            } else if (msg["msg"] == "opponentSuccess") {
                response = R"({"type": "opponentSuccess"})";
            }
            write(session->clientSocket, response.c_str(), response.size() + 1);
            continue;
        }
        auto& request = message.body;

        // 检查 request 格式是否有效
        if (request["type"].is_null()) {
            LOG(WARNING) << "'type' missing; Discard";
            continue;
        } else if (callbackMap.count(request["type"]) == 0) {
            LOG(WARNING) << "Unknown request 'type'; Discard";
            continue;
        } else if (request["type"] != "connect" && request["sessionId"].is_null()) {
            LOG(WARNING) << "No SessionId; Discard";
            continue;
        } else if (request["type"] != "connect" && sessionMap.count(request["sessionId"]) == 0) {
            LOG(WARNING) << "Invalid SessionId; Discard";
            continue;
        }

        Json response;
        // 根据回调函数表，调用相应函数处理游戏逻辑
        callbackMap[request["type"]](response, request);

        // 组装 response
        response["sessionId"] = session->id;

        LOG(INFO) << "Response:" << "\e[90m" << response << "\e[0m";
        // 回复消息给客户端
        std::string re = response.dump();
        write(session->clientSocket, re.c_str(), re.size() + 1);

    }
    LOG(INFO) << "Client thread finished.";
}

void Server::registerGameLogic(std::string type, std::function<void (Json& response, Json& request)> callback) {
    callbackMap[type] = callback;
}

} // namespace Sermo