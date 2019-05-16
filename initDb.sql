CREATE TABLE user(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    usertype INTEGER NOT NULL DEFAULT 0,
    username TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
    nickname TEXT,
    avatar INTEGER,
    level INTEGER DEFAULT 1
);

CREATE TABLE player(
    id INTEGER PRIMARY KEY,
    exprience INTEGER DEFAULT 0,
    achievement INTEGER DEFAULT 0
);

CREATE TABLE examiner(
    id INTEGER PRIMARY KEY,
    number_word INTEGER DEFAULT 0
);

CREATE TABLE question(
    word TEXT NOT NULL UNIQUE,
    level INTEGER NOT NULL DEFAULT 1,
    examiner INTEGER NOT NULL
);