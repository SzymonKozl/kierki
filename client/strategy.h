//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_STRATEGY_H
#define KIERKI_STRATEGY_H

#include "../common/card.h"
#include "../common/common_types.h"

class Strategy {
public:
    Strategy(Hand hand, int mode);
    void reset(Hand hand, int mode);
    Card nextMove(Table const& t);
    Hand& accHand();
private:
    Hand hand;
};

#endif //KIERKI_STRATEGY_H
