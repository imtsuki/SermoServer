#ifndef _MODELS_USER_H_
#define _MODELS_USER_H_

#include <string>

class User {
public:
    enum UserType {
        PLAYER   = 0,
        EXAMINER = 1
    };
private:
    int id;
    std::string username;
    UserType userType;
    std::string nickname;
    std::string password;
    int avatar;
    int level;
protected:

public:
    static User *create() { return new User(); }

    User() = default;

    int getId() const { return id; }
    void setId(const int& i) { id = i; }

    std::string getUsername() const { return username; }
    void setUsername(const std::string& name) { username = name; }

    std::string getPassword() const { return password; }
    void setPassword(const std::string& pass) { password = pass; }

    UserType getUserType() const { return userType; }
    void setUserType(const UserType& type) { userType = type; }

    std::string getNickname() const { return nickname; }
    void setNickname(const std::string& name) { nickname = name; }

    int getAvatar() const { return avatar; }
    void setAvatar(const int& ava) { avatar = ava; }
    
    int getLevel() const { return level; }
    void setLevel(const int& lv) { level = lv; }
};

#endif /* _MODEL_USER_H_ */