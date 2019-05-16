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
    user.setId(-1);

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

    if (user.getId() == -1) {
        return nullptr;
    }


    // 第二步：判断用户类型，进入不同处理逻辑
    switch (user.getUserType()) {
        case User::UserType::PLAYER: {
            auto player = Player::create();
            *player = queryPlayer(user.getId());
            return player;
        }
        break;
        case User::UserType::EXAMINER: {
            auto examiner = Examiner::create();
            *examiner = queryExaminer(user.getId());
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
                LOG(DEBUG) << column_value[i];
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
                player->setExperience(std::stoi(column_value[i]));
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


std::vector<Player> Db::queryPlayerList() {
    std::vector<int> playerIds = queryIdList(User::UserType::PLAYER);
    std::vector<Player> players;
    for (auto id : playerIds) {
        players.push_back(queryPlayer(id));
    }
    return players;

}

std::vector<Examiner> Db::queryExaminerList() {
    std::vector<int> examinerIds = queryIdList(User::UserType::EXAMINER);
    std::vector<Examiner> examiners;
    for (auto id : examinerIds) {
        examiners.push_back(queryExaminer(id));
    }
    return examiners;
}

std::vector<int> Db::queryIdList(User::UserType type) {
    std::ostringstream s;
    std::vector<int> ids;

    s << "SELECT id FROM user WHERE usertype = "
      << type
      << ";";
    
    auto callback = [](void *params, int column_size, char **column_value, char **column_name) -> int {
        auto ids = (std::vector<int> *)params;
        ids->push_back(std::stoi(column_value[0]));
        return 0;
    };

    sqlite3_exec(db, s.str().c_str(), callback, &ids, nullptr);

    return ids;
}

int Db::addWord(std::string word, int examinerId) {
    int level = word.size();
    std::ostringstream s;
    s << "INSERT INTO question VALUES ('"
      << word
      << "',"
      << level
      << ","
      << examinerId
      << ");";
    int error = sqlite3_exec(db, s.str().c_str(), nullptr, nullptr, nullptr);
    if (error) {
        LOG(WARNING) << sqlite3_errmsg(db);
        return 1;
    }
    return 0;
}

int Db::update(int id, const std::string& table, const std::string& key, const std::string& value) {
    std::ostringstream s;
    s << "UPDATE " << table << " "
      << "SET " << key << " = '" << value << "' "
      << "WHERE id = " << id <<";";
    int error = sqlite3_exec(db, s.str().c_str(), nullptr, nullptr, nullptr);
    if (error) {
        LOG(WARNING) << sqlite3_errmsg(db);
        return 1;
    }
    return 0;
}

int Db::update(int id, const std::string& table, const std::string& key, int value) {
    std::ostringstream s;
    s << "UPDATE " << table << " "
      << "SET " << key << " = " << value <<" "
      << "WHERE id = " << id <<";";

    int error = sqlite3_exec(db, s.str().c_str(), nullptr, nullptr, nullptr);
    if (error) {
        LOG(WARNING) << sqlite3_errmsg(db);
        return 1;
    }
    return 0;
}

std::string Db::getWord() {
    std::string query = "SELECT word FROM question ORDER BY RANDOM() LIMIT 1;";
    auto callback = [](void *params, int column_size, char **column_value, char **column_name) -> int {
        auto result = (std::string *)params;
        *result = column_value[0];
        return 0;
    };
    std::string result;
    sqlite3_exec(db, query.c_str(), callback, &result, nullptr);
    return result;

}

std::string Db::getWord(int maxLength) {
    std::ostringstream s;
    s << "SELECT word FROM question WHERE LENGTH(word) < "
      << maxLength
      << " ORDER BY RANDOM() LIMIT 1;";
    
    auto callback = [](void *params, int column_size, char **column_value, char **column_name) -> int {
        auto result = (std::string *)params;
        *result = column_value[0];
        return 0;
    };
    std::string result;
    int error = sqlite3_exec(db, s.str().c_str(), callback, &result, nullptr);
    if (error) {
        LOG(WARNING) << sqlite3_errmsg(db);
    }
    return result;
}


} // namespace Sermo