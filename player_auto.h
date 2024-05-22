//
// Created by szymon on 15.05.24.
//

#ifndef KIERKI_CLIENT_AUTO_H
#define KIERKI_CLIENT_AUTO_H

#include "player.h"
#include "strategy.h"
#include "card.h"
#include "common_types.h"
#include "message.h"
#include "logger.h"

#include "functional"
#include "string"

using chooseCardCb = std::function<void(Card)>;

class PlayerAuto: public Player {
public:
    PlayerAuto();

    void trickMsg(int trickNo, const Table& table) override;
    void takenMsg(Side side, const Table& cards, bool apply) override;
    void scoreMsg(const score_map scores, bool total) override;
    void dealMsg(int trickMode, Hand hand) override;
    void wrongMsg(int trickNo) override;
    void anyMsg(Message message) override;
    void anyCmd(std::string msg) override;
    void setTrickCb(chooseCardCb &&trick_callback);
private:
    Strategy strategy;
    chooseCardCb trickCb;
    Logger logger;
};


#endif //KIERKI_CLIENT_AUTO_H
