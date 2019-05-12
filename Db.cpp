#include "Db.h"
#include <g3log/g3log.hpp>
#include <sstream>

namespace Sermo {

Db *Db::sharedInstance = nullptr;

Db *Db::getInstance() {
    if (!sharedInstance) {
        createSharedInstance();
    }
    return sharedInstance;
}

bool Db::createSharedInstance() {
    sharedInstance = new (std::nothrow) Db();
    sharedInstance->init();
    return true;
}

bool Db::init() {
    return true;
}

bool Db::connect(const std::string& dbFile) {
    int result;
    LOG(INFO) << "libsqlite3 version: " << sqlite3_libversion();
    result = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    if (result) {
        LOG(FATAL) << "Cannot configure sqlite3 to SERIALIZED mode: " << sqlite3_errmsg(db) << "; shutting down now";
    }
    LOG(INFO) << "Configured libsqlite3 to SERIALIZED mode";
    result = sqlite3_open(dbFile.c_str(), &db);
    if (result) {
        LOG(FATAL) << "Cannot connect to database: " << sqlite3_errmsg(db) << "; shutting down now";
    }
    LOG(INFO) << "Connected to " << dbFile;

    return true;
}

void Db::close() {
    delete this;
}

Db::~Db() {
    if (db) {
        int result = sqlite3_close(db);
        if (result) {
            LOG(WARNING) << "Error occured while closing database: " << sqlite3_errmsg(db);
        }
        db = nullptr;
    } else {
        LOG(WARNING) << "No database connected; closing anyhow.";
    }
    LOG(INFO) << "Database closed.";
    Db::sharedInstance = nullptr;
}


int Db::registerUser(const User& user) {
    // 创建用户基本信息
    std::ostringstream s;
    int error;
    std::string query;

    s << "INSERT INTO user (usertype, username, password, nickname, avatar)"
      << "VALUES ("
      << user.getUserType()
      << ",'"
      << user.getUsername()
      << "','"
      << user.getPassword()
      << "','"
      << user.getUsername() // 默认昵称
      << "',"
      << 0                  // 默认头像
      << ");";
    query = s.str();
    error = sqlite3_exec(db, query.c_str(), 0, 0, nullptr);
    if (error) {
        LOG(WARNING) << sqlite3_errmsg(db);
        return 1;
    }
    // 获得用户id
    int id = queryUserId(user.getUsername());
    // 根据用户子类型创建子表项
    // 玩家
    std::string table;
    if (user.getUserType() == User::UserType::PLAYER) {
        table = "player";
    // 出题者
    } else {
        table = "examiner";
    }
    s.clear();
    s.str("");
    s << "INSERT INTO " << table << " (id)"
      << "VALUES ("
      << id
      << ");";
    query = s.str();
    error = sqlite3_exec(db, query.c_str(), 0, 0, nullptr);
    if (error) {
        LOG(WARNING) << sqlite3_errmsg(db);
        return 1;
    }
    return 0;
}

int Db::queryUserId(const std::string& username) {
    std::ostringstream s;
    s << "SELECT id FROM user WHERE username = '"
      << username
      << "';";
    std::string query(s.str());
    auto callback = [](void *params, int column_size, char **column_value, char **column_name) -> int {
        int *id = (int *)params;
        *id = std::stoi(column_value[0]);
        return 0;
    };
    int id;
    int error = sqlite3_exec(db, query.c_str(), callback, &id, nullptr);
    if (error) {
        LOG(WARNING) << sqlite3_errmsg(db);
        return -1;
    }
    return id;

}


User *Db::queryUser(const std::string& username) {
    // 返回用户信息，根据类型不同返回不同的子类
    // 第一步：查询 user 表，获得 User 基类信息
    std::ostringstream s;
    User user;

    s << "SELECT id, usertype FROM user WHERE username = '"
      << username
      << "';";
    std::string query(s.str());

    auto callback = [](void *params, int column_size, char **column_value, char **column_name) -> int {
        User *user = (User *)params;
        for (int i = 0; i < column_size; i++) {
            if (std::string("id") == column_name[i]) {
                user->setId(std::stoi(column_value[i]));
            } else if (std::string("usertype") == column_name[i]) {
                auto type = (User::UserType)std::stoi(column_value[i]);
                user->setUserType(type);
            }
        }
        return 0;
    };

    int error = sqlite3_exec(db, query.c_str(), callback, &user, nullptr);
    if (error) {
        LOG(WARNING) << sqlite3_errmsg(db);
        return nullptr;
    }

    // 第二步：判断用户类型，进入不同处理逻辑
    switch (user.getUserType()) {
        case User::UserType::PLAYER: {

            auto playerN = Player::create();
            *playerN = queryPlayer(user.getId());
            return playerN;

            auto player = Player::create(&user);
            auto playerCallback = [](void *params,int column_size,char **column_value,char **column_name) -> int {
                auto player = (Player *)params;
                for (int i = 0; i < column_size; i++) {
                    if (std::string("exprience") == column_name[i]) {
                        player->setExprience(std::stoi(column_value[i]));
                    } else if (std::string("achievement") == column_name[i]) {
                        player->setAchievement(std::stoi(column_value[i]));
                    }
                }
                return 0;
            };

            std::ostringstream s;
            s << "SELECT * FROM player WHERE id = "
              << user.getId()
              << ";";
            std::string query(s.str());
            
            int error = sqlite3_exec(db, query.c_str(), playerCallback, player, nullptr);
            if (error) {
                LOG(WARNING) << sqlite3_errmsg(db);
                delete player;
                return nullptr;
            }
            return player;
        }
        break;
        case User::UserType::EXAMINER: {

            auto examinerN = Examiner::create();
            *examinerN = queryExaminer(user.getId());
            return examinerN;

            auto examiner = Examiner::create(&user);
            auto examinerCallback = [](void *params,int column_size,char **column_value,char **column_name) -> int {
                auto examiner = (Examiner *)params;
                for (int i = 0; i < column_size; i++) {
                    if (std::string("number_word") == column_name[i]) {
                        examiner->setNumberWord(std::stoi(column_value[i]));
                    }
                }
                return 0;
            };

            std::ostringstream s;
            s << "SELECT * FROM examiner WHERE id = "
              << user.getId()
              << ";";
            std::string query(s.str());
            
            int error = sqlite3_exec(db, query.c_str(), examinerCallback, examiner, nullptr);
            if (error) {
                LOG(WARNING) << sqlite3_errmsg(db);
                delete examiner;
                return nullptr;
            }
            return examiner;
        }
        break;
    }
}

User Db::queryUser(int id) {
    std::ostringstream s;
    User user;

    s << "SELECT * FROM user WHERE id = "
      << id
      << ";";
    
    auto callback = [](void *params, int column_size, char **column_value, char **column_name) -> int {
        User *user = (User *)params;
        for (int i = 0; i < column_size; i++) {
            if (std::string("id") == column_name[i]) {
                user->setId(std::stoi(column_value[i]));
            } else if (std::string("password") == column_name[i]) {
                user->setPassword(column_value[i]);
            } else if (std::string("username") == column_name[i]) {
                user->setUsername(column_value[i]);
            } else if (std::string("usertype") == column_name[i]) {
                auto type = (User::UserType)std::stoi(column_value[i]);
                user->setUserType(type);
            } else if (std::string("nickname") == column_name[i]) {
                user->setNickname(column_value[i]);
            } else if (std::string("avatar") == column_name[i]) {
                user->setAvatar(std::stoi(column_value[i]));
            } else if (std::string("level") == column_name[i]) {
                user->setLevel(std::stoi(column_value[i]));
            }
        }
        return 0;
    };

    sqlite3_exec(db, s.str().c_str(), callback, &user, nullptr);
    return user;
}



Player Db::queryPlayer(int id) {
    std::ostringstream s;

    User user = queryUser(id);
    Player player(&user);

    s << "SELECT * FROM player WHERE id = "
      << id
      << ";";
    
    auto callback = [](void *params, int column_size, char **column_value, char **column_name) -> int {
        auto player = (Player *)params;
        for (int i = 0; i < column_size; i++) {
            if (std::string("exprience") == column_name[i]) {
                player->setExprience(std::stoi(column_value[i]));
            } else if (std::string("achievement") == column_name[i]) {
                player->setAchievement(std::stoi(column_value[i]));
            }
        }
        return 0;
    };

    sqlite3_exec(db, s.str().c_str(), callback, &player, nullptr);
    return player;

}


Examiner Db::queryExaminer(int id) {
    std::ostringstream s;

    User user = queryUser(id);
    Examiner examiner(&user);

    s << "SELECT * FROM examiner WHERE id = "
      << id
      << ";";
    
    auto callback = [](void *params, int column_size, char **column_value, char **column_name) -> int {
        auto examiner = (Examiner *)params;
        for (int i = 0; i < column_size; i++) {
            if (std::string("number_word") == column_name[i]) {
                examiner->setNumberWord(std::stoi(column_value[i]));
            }
        }
        return 0;
    };

    sqlite3_exec(db, s.str().c_str(), callback, &examiner, nullptr);
    return examiner;
}


} // namespace Sermo