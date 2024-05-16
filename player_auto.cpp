//
// Created by szymon on 15.05.24.
//

#include "player_auto.h"
#include "common_types.h"
#include "card.h"

#include "vector"

PlayerAuto::PlayerAuto():
    Player(),
    strategy(std::vector<Card>(), TRICK_PENALTY)
{}

void PlayerAuto::scoreMsg(const std::unordered_map<Side, int> scores, bool total) {}

void PlayerAuto::takenMsg(Side side, const Table &cards) {}

Card PlayerAuto::trickMsg(int trickNo, const Table &table) {
    return strategy.nextMove(table);
}

void PlayerAuto::dealMsg(int trickMode, Hand hand) {
    strategy.reset(hand, trickMode);
}
