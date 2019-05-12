#include <sys/errno.h>

void log_init(int argc, char **argv, char **envp);
void log_shutdown();

#define LOG_FATAL_ERRNO()                               \
    do {                                                \
        LOG(FATAL) << errno << " " << strerror(errno);  \
    } while(0);

#define LOG_WARNING_ERRNO()                             \
    do {                                                \
        LOG(WARNING) << errno << " " << strerror(errno);\
    } while(0);
