#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <cstdio>
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(5678);
    int r = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("connect return %d\n", r);

    Json connectJson = {
        {"type", "connect"}
    };
    write(sock, connectJson.dump().c_str(), connectJson.dump().size() + 1);
    sleep(1);

    char json2[] =
    R"({
        "type": "register",
        "body": {
            "username": "tsuki",
            "password": "e4a1b0f8cc213a1fcad9",
            "userType": 0
        }
    })";
    write(sock, json2, sizeof(json2));

    char json3[] = R"({"type": "logout"})";
    int n = 10000;
    while (n--) {
        write(sock, json3, sizeof(json3));
        sleep(1);
    }
    close(sock);
           // StringBuffer buffer;
        //Writer<StringBuffer> writer(buffer);
        //d.Accept(writer);
        //buffer.GetString();
    return 0;
}