#ifndef _MODELS_PLAYER_H_
#define _MODELS_PLAYER_H_

#include <string>
#include "User.h"

class Player : public User {
private:
    int exprience;
    int achievement;

public:
    static Player *create() { return new Player(); }

    static Player *create(const User *user) {
        return new Player(user);
    }

    Player() = default;

    Player(const User *user) {
        setUsername(user->getUsername());
        setPassword(user->getPassword());
        setUserType(user->getUserType());
        setNickname(user->getNickname());
        setAvatar(user->getAvatar());
        setLevel(user->getLevel());
    }

    int getExprience() const { return exprience; }
    void setExprience(const int& ex) { exprience = ex; }

    int getAchievement() const { return achievement; }
    void setAchievement(const int& achiv) { achievement = achiv; }
};

#endif /* _MODEL_PLAYER_H_ */