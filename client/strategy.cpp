//
// Created by szymon on 15.05.24.
//

#include "strategy.h"

#include <utility>
#include "../common/common_types.h"

Strategy::Strategy(Hand hand, int mode):
    hand(std::move(hand))
{}

Card Strategy::nextMove(const Table &t) {
    if (!t.empty()) {
        color colorReq = t.front().getColor();
        Card cand = hand.front();
        auto iter = hand.begin();
        auto to_erase = iter;
        for (; iter != hand.end(); iter++) {
            Card c = *iter;
            if (cand.getColor() != colorReq && c.getColor() == colorReq) {
                to_erase = iter;
                cand = c;
            }
        }
        hand.erase(to_erase);
        return cand;
    }
    else {
        Card card = hand.back();
        hand.pop_back();
        return card;
    }
}

void Strategy::reset(Hand new_hand, int) {
    hand = std::move(new_hand);
}

Hand &Strategy::accHand() {
    return hand;
}
