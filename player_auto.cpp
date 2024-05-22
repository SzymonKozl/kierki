//
// Created by szymon on 15.05.24.
//

#include "player_auto.h"
#include "utils.h"
#include "common_types.h"
#include "card.h"

#include "iostream"
#include "vector"

PlayerAuto::PlayerAuto():
    Player(),
    strategy(std::vector<Card>(), TRICK_PENALTY),
    trickCb(),
    logger(std::cout, false)
{}

void PlayerAuto::scoreMsg(const std::unordered_map<Side, int> scores, bool total) {}

void PlayerAuto::trickMsg(int trickNo, const Table &table) {
    putCb(strategy.nextMove(table));
}

void PlayerAuto::dealMsg(int trickMode, Hand hand) {
    strategy.reset(hand, trickMode);
}

void PlayerAuto::wrongMsg(int trickNo) {}

void PlayerAuto::anyCmd(std::string msg) {

}

void PlayerAuto::anyMsg(Message message) {
    logger.log(message);
}

void PlayerAuto::setTrickCb(chooseCardCb &&trick_callback) {
    trickCb = std::move(trick_callback);
}

void PlayerAuto::takenMsg(Side side, const Table &cards, bool apply) {
    if (apply) {
        rmIntersection(hand, cards);
    }
}
