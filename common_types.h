//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_COMMON_TYPES_H
#define KIERKI_COMMON_TYPES_H

#include "card.h"

#include "vector"
#include "unordered_map"

enum Side {
    N = 'n',
    S = 's',
    W = 'w',
    E = 'e',
    _SIDE_NULL = 0
};

using Hand = std::vector<Card>;
using Table = std::vector<Card>;
using game_scenario = std::vector<std::pair<int, std::unordered_map<Side, Hand>>>;

#define IO_ERR_INTERNAL -11
#define IO_ERR_EXTERNAL -12

enum RoundType {
    TRICK_PENALTY = 1,
    HEART_PENALTY = 2,
    QUEEN_PENALTY = 3,
    KING_JACK_PENALTY = 4,
    HEART_KING_PENALTY = 5,
    SEVENTH_LAST_TRICK_PENALTY = 6,
    EVERYTHING = 7
};

#endif //KIERKI_COMMON_TYPES_H
