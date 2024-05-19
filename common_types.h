//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_COMMON_TYPES_H
#define KIERKI_COMMON_TYPES_H

#include "card.h"

#include "vector"
#include "unordered_map"
#include "tuple"
#include "stdint.h"

enum Side {
    N = 'N',
    E = 'E',
    S = 'S',
    W = 'W',
    SIDE_NULL_ = 0
};

#define sides_ {N, S, W, E}

enum RoundType {
    TRICK_PENALTY = 1,
    HEART_PENALTY = 2,
    QUEEN_PENALTY = 3,
    KING_JACK_PENALTY = 4,
    HEART_KING_PENALTY = 5,
    SEVENTH_LAST_TRICK_PENALTY = 6,
    EVERYTHING = 7
};

struct errInfo {
    const std::string& call;
    int errnoVal;
    int errType;
};


using Hand = std::vector<Card>;
using Table = std::vector<Card>;
using table_state = std::unordered_map<Side, Hand>;
using score_map = std::unordered_map<Side, int>;
using game_scenario = std::vector<std::tuple<RoundType, table_state, Side>>;
using net_address = std::pair<uint16_t, std::string>;
#endif //KIERKI_COMMON_TYPES_H
