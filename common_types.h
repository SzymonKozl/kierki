//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_COMMON_TYPES_H
#define KIERKI_COMMON_TYPES_H

#include "card.h"

#include "vector"
#include "unordered_map"
#include "tuple"

enum Side {
    N = 'n',
    S = 's',
    W = 'w',
    E = 'e',
    _SIDE_NULL = 0
};

Side sides_[] = {N, S, W, E};

enum RoundType {
    TRICK_PENALTY = 1,
    HEART_PENALTY = 2,
    QUEEN_PENALTY = 3,
    KING_JACK_PENALTY = 4,
    HEART_KING_PENALTY = 5,
    SEVENTH_LAST_TRICK_PENALTY = 6,
    EVERYTHING = 7
};


using Hand = std::vector<Card>;
using Table = std::vector<Card>;
using table_state = std::unordered_map<Side, Hand>;
using game_scenario = std::vector<std::tuple<RoundType, table_state, Side>>;

#define IO_ERR_INTERNAL -11
#define IO_ERR_EXTERNAL -12
#endif //KIERKI_COMMON_TYPES_H
