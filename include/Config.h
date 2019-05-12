// libconfig wrapper.
#ifndef CONFIG_H
#define CONFIG_H

#include <libconfig.h++>

namespace Sermo {

class Config {
public:
    static libconfig::Config *getInstance();
    static bool createSharedInstance();
private:
    static libconfig::Config *sharedInstance;
};

} // namespace Sermo

#endif
