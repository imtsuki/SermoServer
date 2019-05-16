#include "log.h"

#include <iostream>
#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include <g3log/logmessage.hpp>
#include <sys/stat.h>

std::unique_ptr<g3::LogWorker> worker;

struct StdoutSink {
    enum Color { YELLOW = 33, RED = 31, GREEN = 32, WHITE = 97, BLUE = 96 };

    Color getColor(const LEVELS level) const {
        if (level.value == WARNING.value)   return YELLOW;
        if (level.value == DEBUG.value)     return GREEN;
        if (g3::internal::wasFatal(level))  return RED;

        return BLUE;
    }
  
    void ReceiveLogMessage(g3::LogMessageMover logMessage) {
        auto level = logMessage.get()._level;

        std::cout
            << logMessage.get().timestamp({"%H:%M:%S.%f3"}) << " "
            << logMessage.get().threadID() << " "
            << "\033[" << getColor(level) << ";1m"
            << logMessage.get().level() << " "
            << "\033[m"
            << logMessage.get().message() << std::endl;
    }
};

void log_init(int argc, char **argv, char **envp) {
    mkdir("./log/", 0755);
    worker = g3::LogWorker::createLogWorker();
    worker->addDefaultLogger(argv[0], "./log/");
    worker->addSink(std::make_unique<StdoutSink>(),
                    &StdoutSink::ReceiveLogMessage);
    g3::initializeLogging(worker.get());
}
