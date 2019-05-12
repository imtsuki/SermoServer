#include "Server.h"
#include "Db.h"
#include "User.h"
#include <g3log/g3log.hpp>

namespace Sermo {

void Server::registerAllGameLogic() {

    registerGameLogic(
        "connect",
        [&](Json& response, Json& request) {
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "register",
        [&](Json& response, Json& request) {

            auto user = User();
            user.setUsername(request["body"]["username"]);
            user.setPassword(request["body"]["password"]);
            user.setUserType(request["body"]["userType"]);
            // 数据库插入注册信息
            auto db = Db::getInstance();
            int err = db->registerUser(user);
            // 处理用户名重复逻辑
            if (err) {
                response["status"] = "error";
                response["errorMsg"] = "这个用户名已经被占用了， 换一个吧";
            }
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "login",
        [&](Json& response, Json& request) {
            auto session = sessionMap[request["sessionId"]];
            // 数据库查询该用户信息
            auto db = Db::getInstance();
            session->user = db->queryUser(request["body"]["username"]);
            // 处理用户名不存在逻辑
            if (session->user == nullptr) {
                response["status"] = "error";
                response["errorMsg"] = "用户不存在";
                return;
            }
            // TODO: 处理用户登录重复逻辑
            if (false) {
                response["status"] = "error";
                response["errorMsg"] = "该用户已经登陆了";
                delete session->user;
                session->user = nullptr;
                return;
            }
            // 处理密码不相符逻辑
            if (session->user->getPassword() != request["body"]["password"]) {
                response["status"] = "error";
                response["errorMsg"] = "密码错误";
                delete session->user;
                session->user = nullptr;
                return;
            }

            response["status"] = "success";
            response["userType"] = session->user->getUserType();
            sessionMap[request["sessionId"]]->status = Session::Status::LOGIN;
        }
    );

    registerGameLogic(
        "logout",
        [&](Json& response, Json& request) {
            sessionMap[request["sessionId"]]->status = Session::Status::CONNECTED;
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "info",
        [&](Json& response, Json& request) {
            // 返回本用户信息
            auto session = sessionMap[request["sessionId"]];
            // TODO: isLogin()
            if (session->status != Session::Status::CONNECTED) {
                response["status"] = "success";
                response["body"]["username"]= session->user->getUsername();
                response["body"]["userType"]= session->user->getUserType();
                response["body"]["nickname"]= session->user->getNickname();
                response["body"]["avatar"]= session->user->getAvatar();
                response["body"]["level"]= session->user->getLevel();

                if (session->user->getUserType() == User::UserType::PLAYER) {
                    auto player = (Player *)session->user;
                    response["body"]["exprience"] = player->getExprience();
                    response["body"]["achievement"] = player->getAchievement();

                } else {
                    auto examiner = (Examiner *)session->user;
                    response["body"]["number_word"] = examiner->getNumberWord();
                }
            } else {
                response["status"] = "error";
                response["errorMsg"] = "没有登录";
            }
        }
    );

    registerGameLogic(
        "playerList",
        [&](Json& response, Json& request) {
            // TODO: 返回所有玩家列表（JSON 数组的形式）
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "examinerList",
        [&](Json& response, Json& request) {
            // TODO: 返回所有出题者列表（JSON 数组的形式）
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "addWord",
        [&](Json& response, Json& request) {
            // TODO: 出题者增加单词
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "pve",
        [&](Json& response, Json& request) {
            // TODO: 返回一个单机关卡的所有信息
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "pvp",
        [&](Json& response, Json& request) {
            // TODO: 双人对战
            response["status"] = "success";
        }
    );



}

} // namespace Sermo