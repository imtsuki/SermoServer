#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <thread>
#include <vector>
#include <map>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "Session.h"

using Json = nlohmann::json;

namespace Sermo {

class Server {
public:
    static Server *getInstance();

    static bool createSharedInstance();

    void listenOn(int port);

private:
    void init();

    void createListenSocket();

    void bindToPort(int port);

    void startMainLoop();

    void startSessionEventLoop(Session *session);

    void acceptConnection(int *clientSocket, sockaddr_in *clientAddress);

    void dispatchClient(int clientSocket, sockaddr_in clientAddress);

    void registerSession(Session *session);

    void tcpMessageReceiver(Session *session);

    void registerGameLogic(std::string type, std::function<void (Json& response, Json& request)> callback);

    void registerAllGameLogic();

private:
    static Server *sharedInstance;
    int listenSocket = 0;
    sockaddr_in listenAddress;
    int listenPort = 0;
    std::vector<std::thread> threads;
    std::map<int, Session *> sessionMap;
    std::unordered_map<std::string, std::function<void (Json& response, Json& request)> > callbackMap;
};

} // namespace Sermo