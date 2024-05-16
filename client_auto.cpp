//
// Created by szymon on 15.05.24.
//

#include "client_auto.h"
#include "common_types.h"
#include "card.h"

#include "vector"

ClientAuto::ClientAuto():
    Client(),
    strategy(std::vector<Card>(), TRICK_PENALTY)
{}

void ClientAuto::scoreMsg(const std::unordered_map<Side, int> scores, bool total) {}

void ClientAuto::takenMsg(Side side, const Table &cards) {}

Card ClientAuto::trickMsg(int trickNo, const Table &table) {
    return strategy.nextMove(table);
}

void ClientAuto::dealMsg(int trickMode, Hand hand) {
    strategy.reset(hand, trickMode);
}
