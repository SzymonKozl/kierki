//
// Created by szymon on 15.05.24.
//

#ifndef KIERKI_CLIENT_H
#define KIERKI_CLIENT_H

#include "common_types.h"

#include "unordered_map"

class Player {
public:
    Player();
    virtual Card trickMsg(int trickNo, const Table& table);
    virtual void takenMsg(Side side, const Table& cards);
    virtual void scoreMsg(const std::unordered_map<Side, int> scores, bool total);
    virtual void dealMsg(int trickMode, Hand hand);
protected:
    Hand hand;
};


#endif //KIERKI_CLIENT_H
