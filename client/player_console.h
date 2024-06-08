//
// Created by szymon on 25.05.24.
//

#ifndef KIERKI_PLAYER_CONSOLE_H
#define KIERKI_PLAYER_CONSOLE_H

#include "player.h"
#include "../common/common_types.h"

#include "vector"
#include "string"

class PlayerConsole: public Player {
public:
    explicit PlayerConsole(Side side);

    void trickMsg(int trickNo, const Table &table) override;

    void takenMsg(Side side, const Table &cards, int trickNo, bool apply) override;

    void scoreMsg(score_map scores, bool total) override;

    void dealMsg(int trickMode, const Hand& hand, Side starting) override;

    void wrongMsg(int trickNo) override;

    void anyMsg(Message message) override;

    void anyCmd(std::string msg) override;

private:
    void busyMsg(const std::vector<Side> &taken) override;

    Hand localHand;
    std::vector<std::pair<Side, Table>> takenTricks;
};

#endif //KIERKI_PLAYER_CONSOLE_H
