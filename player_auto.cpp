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
    nextTrick(0),
    strategy(std::vector<Card>(), TRICK_PENALTY),
    trickCb(),
    logger(std::cout, false)
{}

void PlayerAuto::scoreMsg(const std::unordered_map<Side, int> scores, bool total) {}

void PlayerAuto::trickMsg(int trickNo, const Table &table) {
    if (trickNo < nextTrick) return;
    nextTrick ++;
    Card c = strategy.nextMove(table);
    lastCards.push_back(std::make_shared<Card>(c.getValue(), c.getColor()));
    putCb(c);
}

void PlayerAuto::dealMsg(int trickMode, const Hand& hand, Side starting) {
    nextTrick = 1;
    strategy.reset(hand, trickMode);
}

void PlayerAuto::wrongMsg(int trickNo) {
    nextTrick = trickNo;
    if (!lastCards.empty()) {
        strategy.accHand().push_back(*lastCards.front());
        lastCards.pop_front();
    }
    else {
        std::cerr << "warn: should remove card\n";
    }
}

void PlayerAuto::anyCmd(std::string msg) {

}

void PlayerAuto::anyMsg(Message message) {
    logger.log(message);
}

void PlayerAuto::setTrickCb(chooseCardCb &&trick_callback) {
    trickCb = std::move(trick_callback);
}

void PlayerAuto::takenMsg(Side side, const Table &cards, int trickNo, bool apply) {
    if (apply) {
        rmIntersection(strategy.accHand(), cards);
    }
    else {
        if (!lastCards.empty()) {
            lastCards.pop_front();
        }
        else {
            std::cerr << "warn: should remove card\n";
        }
    }
}

void PlayerAuto::busyMsg(const std::vector<Side> &taken) {

}
