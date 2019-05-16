#ifndef _MODELS_EXAMINER_H_
#define _MODELS_EXAMINER_H_

#include <string>
#include "User.h"

class Examiner : public User {
private:
    int number_word;

public:
    static Examiner *create() { return new Examiner(); }

    static Examiner *create(const User *user) {
        return new Examiner(user);
    }

    Examiner() = default;

    Examiner(const User *user) {
        setId(user->getId());
        setUsername(user->getUsername());
        setPassword(user->getPassword());
        setUserType(user->getUserType());
        setNickname(user->getNickname());
        setAvatar(user->getAvatar());
        setLevel(user->getLevel());
    }

    int getNumberWord() const { return number_word; }
    void setNumberWord(const int& nw) { number_word = nw; }
};

#endif /* _MODELS_EXAMINER_H_ */