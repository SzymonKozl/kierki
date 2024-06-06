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
    explicit PlayerAuto(Side side);

    void trickMsg(int trickNo, const Table& table) override;
    void takenMsg(Side side, const Table &cards, int trickNo, bool apply) override;
    void scoreMsg(score_map scores, bool total) override;
    void dealMsg(int trickMode, const Hand& hand, Side starting) override;
    void wrongMsg(int trickNo) override;
    void anyMsg(Message message) override;
    void anyCmd(std::string msg) override;
    void setTrickCb(chooseCardCb &&trick_callback);

    void busyMsg(const std::vector<Side> &taken) override;

private:
    int nextTrick;
    Strategy strategy;
    chooseCardCb trickCb;
    Logger logger;
};


#endif //KIERKI_CLIENT_AUTO_H
