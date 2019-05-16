#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string>
#include <nlohmann/json.hpp>
#include "MessageQueue.h"
#include "User.h"

namespace Sermo {

typedef int SessionId;

struct Session {
    enum Status {CONNECTED, LOGIN, PVE, DISCONNECTED, DIED};
    SessionId id;
    Status status;
    User *user;
    std::string pvpWord;
    Session* opponent;

    int clientSocket;
    sockaddr_in clientAddress;
    MessageQueue messageQueue;

    Session() = default;
    Session(int socket, sockaddr_in address) 
        : clientSocket(socket), clientAddress(address) { }
    
    bool init(int socket, sockaddr_in address) {
        clientSocket = socket;
        clientAddress = address;
        return true;
    }
    
    static Session *create(int clientSocket, sockaddr_in clientAddress) {
        Session *pRet = new (std::nothrow) Session();
        if (pRet && pRet->init(clientSocket, clientAddress)) {
            return pRet;
        } else {
            delete pRet;
            pRet = nullptr;
            return nullptr;
        }
    }
};

} // namespace Sermo