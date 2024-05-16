//
// Created by szymon on 15.05.24.
//

#ifndef KIERKI_CLIENT_AUTO_H
#define KIERKI_CLIENT_AUTO_H

#include "player.h"
#include "strategy.h"

class PlayerAuto: public Player {
public:
    PlayerAuto();

    Card trickMsg(int trickNo, const Table& table) override;
    void takenMsg(Side side, const Table& cards) override;
    void scoreMsg(const std::unordered_map<Side, int> scores, bool total) override;
    void dealMsg(int trickMode, Hand hand) override;
private:
    Strategy strategy;
};


#endif //KIERKI_CLIENT_AUTO_H
