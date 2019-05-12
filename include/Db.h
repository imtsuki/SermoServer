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

    int queryUserId(const std::string& username);

private:
    static bool createSharedInstance();

    static Db *sharedInstance;
    sqlite3 *db;
};

} // namespace Sermo

#endif