#include <string>
#include <nlohmann/json.hpp>
#include <g3log/g3log.hpp>

using Json = nlohmann::json;

namespace Sermo {

struct Message {
    enum MSGTYPE {
        REQUEST,
        NOTIFICATION,
        CLOSE
    };

    MSGTYPE type;
    Json body;

    Message() = default;

    Message(MSGTYPE type, const std::string& raw) : type(type) {
        try {
            body = Json::parse(raw);
        } catch (Json::exception& e) {
            LOG(WARNING) << e.what();
            body = Json();
        }
    }

    Message(MSGTYPE type) : type(type) { }
};

} // namespace Sermo