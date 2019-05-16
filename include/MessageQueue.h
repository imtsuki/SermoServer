#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "Message.h"

namespace Sermo {

class MessageQueue {
private:
    std::queue<Message> queue;
    std::mutex accessLock;
    std::condition_variable avaliable;
public:

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
};

} // namespace Sermo

#endif