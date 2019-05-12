#include <string>
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

namespace Sermo {

struct Message {
    enum MSGTYPE {
        REQUEST,
        NOTIFICATION
    };

    MSGTYPE type;
    Json body;

    Message() = default;

    Message(MSGTYPE type, const std::string& raw) : type(type) {
        body = Json::parse(raw);
    }
};

} // namespace Sermo