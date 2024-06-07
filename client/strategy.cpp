//
// Created by szymon on 15.05.24.
//

#include "strategy.h"

Strategy::Strategy(Hand hand, int):
    hand(std::move(hand))
{}

Card Strategy::nextMove(const Table &t) {
    if (!t.empty()) {
        Color colorReq = t.front().getColor();
        Card cand = hand.front();
        auto iter = hand.begin();
        auto toErase = iter;
        for (; iter != hand.end(); iter++) {
            Card c = *iter;
            if (cand.getColor() != colorReq && c.getColor() == colorReq) {
                toErase = iter;
                cand = c;
            }
        }
        hand.erase(toErase);
        return cand;
    }
    else {
        Card card = hand.back();
        hand.pop_back();
        return card;
    }
}

void Strategy::reset(Hand newHand, int) {
    hand = std::move(newHand);
}

Hand &Strategy::accHand() {
    return hand;
}
