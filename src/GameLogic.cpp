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
            if (request["body"]["username"] == "" || request["body"]["password"] == "") {
                response["status"] = "error";
                response["errorMsg"] = "用户名或密码为空";
                return;
            }

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
                return;
            }
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "login",
        [&](Json& response, Json& request) {
            auto session = sessionMap[request["sessionId"]];
            if (request["body"]["username"] == "" || request["body"]["password"] == "") {
                response["status"] = "error";
                response["errorMsg"] = "用户名或密码为空";
                return;
            }
            // 数据库查询该用户信息
            auto db = Db::getInstance();
            session->user = db->queryUser((std::string)request["body"]["username"]);
            // 处理用户名不存在逻辑
            if (session->user == nullptr) {
                response["status"] = "error";
                response["errorMsg"] = "用户不存在";
                return;
            }
            // 处理用户登录重复逻辑
            if (onlineSet.count(session->user->getId())) {
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
            onlineSet[session->user->getId()] = session;
            sessionMap[request["sessionId"]]->status = Session::Status::LOGIN;
        }
    );

    registerGameLogic(
        "logout",
        [&](Json& response, Json& request) {
            auto session = sessionMap[request["sessionId"]];
            sessionMap[request["sessionId"]]->status = Session::Status::CONNECTED;
            onlineSet.erase(session->user->getId());
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
                    response["body"]["experience"] = player->getExperience();
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
            // 返回所有玩家列表（JSON 数组的形式）
            auto db = Db::getInstance();
            auto playerList = db->queryPlayerList();
            for (auto& player : playerList) {
                Json playerJson;
                playerJson["username"] = player.getUsername();
                playerJson["nickname"] = player.getNickname();
                playerJson["avatar"] = player.getAvatar();
                playerJson["level"] = player.getLevel();
                
                playerJson["experience"] = player.getExperience();
                playerJson["achievement"] = player.getAchievement();
                playerJson["online"] = onlineSet.count(player.getId()) == 1;
                response["body"].push_back(playerJson);
            }
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "examinerList",
        [&](Json& response, Json& request) {
            // 返回所有出题者列表（JSON 数组的形式）
            auto db = Db::getInstance();
            auto examinerList = db->queryExaminerList();
            for (auto& examiner : examinerList) {
                Json examinerJson;
                examinerJson["username"] = examiner.getUsername();
                examinerJson["nickname"] = examiner.getNickname();
                examinerJson["avatar"] = examiner.getAvatar();
                examinerJson["level"] = examiner.getLevel();
                examinerJson["numberWord"] = examiner.getNumberWord();
                response["body"].push_back(examinerJson);
            }
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "addWord",
        [&](Json& response, Json& request) {
            // 出题者增加单词
            auto session = sessionMap[request["sessionId"]];
            auto db = Db::getInstance();
            // TODO: isLogin()
            if (session->status != Session::Status::LOGIN) {
                response["status"] = "error";
                response["errorMsg"] = "您还未登录，不能出题";
                return;
            }
            if (session->user->getUserType() != User::UserType::EXAMINER) {
                response["status"] = "error";
                response["errorMsg"] = "您不是出题者，不能出题";
                return;
            }
            if (request["body"]["word"] == "") {
                response["status"] = "error";
                response["errorMsg"] = "单词为空！";
                return;
            }
            auto examiner = (Examiner *)session->user;
            int id = examiner->getId();
            int error = db->addWord(request["body"]["word"], id);
            if (error) {
                response["status"] = "error";
                response["errorMsg"] = "单词与题库中已有重复，请换一个单词";
                return;
            }
            // 出题者出题数目更新，升级
            int num = examiner->getNumberWord();
            int level = examiner->getLevel();
            num++;
            if (num == level * 4) {
                level++;
            }
            examiner->setNumberWord(num);
            examiner->setLevel(level);
            db->update(id, "user", "level", level);
            db->update(id, "examiner", "number_word", num);
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "pve",
        [&](Json& response, Json& request) {
            // 返回一个单机关卡的所有信息
            auto session = sessionMap[request["sessionId"]];
            session->status = Session::Status::PVE;
            auto db = Db::getInstance();
            int roundNumber = request["body"]["round"];
            int wordNumber = roundNumber / 3 + 1;
            while (wordNumber--) {
                response["body"].push_back(db->getWord(roundNumber + 3));
            }
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "pveResult",
        [&](Json& response, Json& request) {
            // 单机关卡结算
            auto session = sessionMap[request["sessionId"]];
            auto id = session->user->getId();
            auto db = Db::getInstance();
            auto player = ((Player *)session->user);
            if (request["body"]["isPassed"]) {
                LOG(DEBUG) << "Updating";
                int newAchievement = request["body"]["round"];
                player->setAchievement(newAchievement);
                db->update(id, "player", "achievement", player->getAchievement());
                int exp = request["body"]["exp"];
                player->setExperience(player->getExperience() + exp);
                player->setLevel(player->getExperience() / 100 + 1);
                db->update(id, "player", "exprience", player->getExperience());
                
                db->update(id, "user", "level", player->getLevel());
            }
            session->status = Session::Status::LOGIN;
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "pvp",
        [&](Json& response, Json& request) {
            // TODO: 双人对战
            auto session = sessionMap[request["sessionId"]];
            auto id = session->user->getId();
            auto db = Db::getInstance();
            int opponentId = db->queryUserId(request["body"]["opponent"]);
            if (id == opponentId) {
                response["status"] = "error";
                response["errorMsg"] = "不能挑战自己！";
                return;
            }
            if (!onlineSet.count(opponentId)) {
                response["status"] = "error";
                response["errorMsg"] = "对手没有上线！";
                return;
            }
            session->pvpWord = db->getWord();
            auto opponent = onlineSet[opponentId];
            session->opponent = opponent;
            Json message;
            message = {
                {"msg", "invite"},
                {"opponentId", id}
            };
            opponent->messageQueue.pushMessage(Message(Message::MSGTYPE::NOTIFICATION, message.dump()));
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "acceptInvite",
        [&](Json& response, Json& request) {
            // TODO: 接受邀请
            auto session = sessionMap[request["sessionId"]];
            auto id = session->user->getId();
            auto db = Db::getInstance();
            auto opponent = session->opponent;
            response["body"]["pvpWord"] = session->pvpWord;
            response["status"] = "success";
            Json message;
            message = {
                {"msg", "opponentAccepted"}
            };
            opponent->messageQueue.pushMessage(Message(Message::MSGTYPE::NOTIFICATION, message.dump()));
            LOG(DEBUG) << "Come HERE";
        }
    );
    registerGameLogic(
        "pvpSuccess",
        [&](Json& response, Json& request) {
            // 对战胜利
            auto session = sessionMap[request["sessionId"]];
            auto id = session->user->getId();
            auto db = Db::getInstance();
            auto opponent = session->opponent;
            Json message;
            message = {
                {"msg", "opponentSuccess"}
            };
            opponent->messageQueue.pushMessage(Message(Message::MSGTYPE::NOTIFICATION, message.dump()));
            response["status"] = "success";
        }
    );

    registerGameLogic(
        "pvpResult",
        [&](Json& response, Json& request) {
            // 双人关卡结算
            auto session = sessionMap[request["sessionId"]];
            auto id = session->user->getId();
            auto db = Db::getInstance();
            auto player = ((Player *)session->user);
            if (request["body"]["isPassed"]) {
                LOG(DEBUG) << "Updating";
                int exp = request["body"]["exp"];
                player->setExperience(player->getExperience() + exp);
                player->setLevel(player->getExperience() / 100 + 1);
                db->update(id, "player", "exprience", player->getExperience());
                db->update(id, "user", "level", player->getLevel());
            }
            session->status = Session::Status::LOGIN;
            response["status"] = "success";
        }
    );



}

} // namespace Sermo