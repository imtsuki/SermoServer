#include "Config.h"
#include <g3log/g3log.hpp>

namespace Sermo {

libconfig::Config *Config::sharedInstance = nullptr;

libconfig::Config *Config::getInstance() {
    if (!sharedInstance) {
        createSharedInstance();
    }
    return sharedInstance;
}

bool Config::createSharedInstance() {
    sharedInstance = new (std::nothrow) libconfig::Config();
    LOG(INFO) << "libconfig++ version: " 
        << LIBCONFIGXX_VER_MAJOR << "."
        << LIBCONFIGXX_VER_MINOR << "."
        << LIBCONFIGXX_VER_REVISION;
    return true;
}

}
