#ifndef DB_H
#define DB_H

#include <string>
#include <sqlite3.h>
#include <vector>
#include "User.h"
#include "Player.h"
#include "Examiner.h"

namespace Sermo {

class Db {
public:
    ~Db();
    static Db *getInstance();
    bool init();
    bool connect(const std::string&);
    void close();

    int registerUser(const User&);
    User *queryUser(const std::string& username);
    
    User queryUser(int id);
    Player queryPlayer(int id);
    Examiner queryExaminer(int id);

    std::vector<Player> queryPlayerList();
    std::vector<Examiner> queryExaminerList();
    std::vector<int> queryIdList(User::UserType type);

    int addWord(std::string word, int examinerId);


    int queryUserId(const std::string& username);

    int update(int id, const std::string& table, const std::string& key, const std::string& value);
    int update(int id, const std::string& table, const std::string& key, int value);

    std::string getWord();
    std::string getWord(int maxLength);

private:
    static bool createSharedInstance();

    static Db *sharedInstance;
    sqlite3 *db;
};

} // namespace Sermo

#endif