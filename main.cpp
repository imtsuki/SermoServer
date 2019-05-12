#include "log.h"
#include "Server.h"
#include "Config.h"
#include "Db.h"
#include <g3log/g3log.hpp>

void signalHandler(int signum) {
    Sermo::Db::getInstance()->close();
    exit(signum);
}

int main(int argc, char **argv, char **envp) {
    using namespace Sermo;
    log_init(argc, argv, envp);
    auto config = Config::getInstance();
    config->readFile("./sermo.conf");
    LOG(INFO) << R"(
      ___           ___           ___           ___           ___
     /  /\         /  /\         /  /\         /  /\         /  /\
    /  /::\       /  /::\       /  /::\       /  /::|       /  /::\
   /__/:/\:\     /  /:/\:\     /  /:/\:\     /  /:|:|      /  /:/\:\
  _\_ \:\ \:\   /  /::\ \:\   /  /::\ \:\   /  /:/|:|__   /  /:/  \:\
 /__/\ \:\ \:\ /__/:/\:\ \:\ /__/:/\:\_\:\ /__/:/_|::::\ /__/:/ \__\:\
 \  \:\ \:\_\/ \  \:\ \:\_\/ \__\/~|::\/:/ \__\/  /~~/:/ \  \:\ /  /:/
  \  \:\_\:\    \  \:\ \:\      |  |:|::/        /  /:/   \  \:\  /:/
   \  \:\/:/     \  \:\_\/      |  |:|\/        /  /:/     \  \:\/:/
    \  \::/       \  \:\        |__|:|~        /__/:/       \  \::/
     \__\/         \__\/         \__\|         \__\/         \__\/
)";


    std::string version = config->lookup("version");
    LOG(INFO) << "SermoServer v" << version;
    LOG(INFO) << "build: " << __DATE__ << "  " << __TIME__;

    //signal(SIGINT, signalHandler);

    LOG(INFO) << "Starting server...";
    auto server = Server::getInstance();
    server->listenOn(config->lookup("server.port"));

    return 0;
}