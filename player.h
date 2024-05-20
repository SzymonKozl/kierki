//
// Created by szymon on 15.05.24.
//

#ifndef KIERKI_PLAYER_H
#define KIERKI_PLAYER_H

#include "common_types.h"
#include "message.h"

#include "unordered_map"
#include "functional"

using putCardCb = std::function<void(Card)>;
using cardNeededCb = std::function<bool()>;

class Player {
public:
    Player();
    void setup(putCardCb putCardCallback, cardNeededCb cardNeededCallback);
    virtual void trickMsg(int trickNo, const Table& table) = 0;
    virtual void takenMsg(Side side, const Table& cards, bool apply) = 0;
    virtual void scoreMsg(const score_map scores, bool total) = 0;
    virtual void dealMsg(int trickMode, Hand hand) = 0;
    virtual void wrongMsg(int trickNo) = 0;
    virtual void anyMsg(Message message) = 0;
    virtual void anyCmd(std::string msg) = 0;
protected:
    Hand hand;
    putCardCb putCb;
    cardNeededCb checkCardCb;
};


#endif //KIERKI_PLAYER_H
